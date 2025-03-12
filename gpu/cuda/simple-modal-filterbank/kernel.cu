
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "cuComplex.h"

#include <stdio.h>

#include <windows.h>
#include <tchar.h>

// Synchronization primitives for communicating with plugin client.
// (or any other process that uses these named objects).
TCHAR szName[] = TEXT("Local\\GPUModalBankMem");
TCHAR szNameSemaphore[] = TEXT("Local\\GPUModalBankSemaphore");
TCHAR szNameSemaphoreGPU[] = TEXT("Local\\GPUModalBankSemaphoreGPU");
#define SHAREDMEMSIZE 1024*512*2

#define BUFFERSIZE 256
constexpr int NDRUMS = 10;
constexpr int NMODES = NDRUMS * 1024;
constexpr int NWARPS = 320;  // 10240/32

static float host_samplebuffer[BUFFERSIZE*NWARPS*2];

// Shared Memory Layout
// ----- First Section: Input parameters -----
// 10000 modes *
struct ModeInfo {
	bool enabled;
	bool reset;

	bool amp_changed;
	float amp_real;
	float amp_imag;
	
	float damp;
	float freq;
	bool freq_changed;
};  // approximately 8 * 4 bytes = 32 bytes
// all modes total = 320000 = 320KB

// ----- Drum info ----
// Cleanup: should reorder these sections, but these were added chronologically.
// 10 drums * 8 controllable params per drum.

/// ----- Input State -----
// NDRUMS times BUFFERSIZE for inputs. = 10240 = 10KB

// ----- Third Section: Audio output to Host -----
// 4 bytes per sample * 1024 buffer size (supports stereo@512) = 4K


__device__ __forceinline__ cuFloatComplex custom_cexpf(cuFloatComplex z) {
	cuComplex res;
	float t = expf(z.x);
	sincosf(z.y, &res.y, &res.x);
	res.x *= t;
	res.y *= t;
	return res;
}

__global__ void filterbankKernel(float *yprev, const ModeInfo *mi, const float* drumInfo, const float* input, float* output) {
	int i = threadIdx.x + blockIdx.x * blockDim.x;
	int whichwarp = (int)(i / 32);
	bool is_first_thread_in_warp = (i % 32) == 0;

	// Init - pull from shared memory.
	cuComplex y;
	y.x = yprev[2 * i];
	y.y = yprev[2 * i + 1];

	if (mi[i].reset) {
		y.x = 0.0f;
		y.y = 0.0f;
	}

	cuComplex input_amp;
	input_amp.x = mi[i].amp_real;
	input_amp.y = mi[i].amp_real;

	cuComplex input_complex;

	cuComplex exp_term;
	{
		// regenerate
		cuComplex e_stuff;
		e_stuff.x = -mi[i].damp;
		e_stuff.y = mi[i].freq;
		exp_term = custom_cexpf(e_stuff);
	}

	int whichDrum = (int)(i / 1024);
	float pan = drumInfo[whichDrum * 8 + 0];

	const float *input_base = input + (BUFFERSIZE*whichDrum);
	// Main loop - spin for enough cycles to generate the whole buffer.
	for (int samp = 0; samp < BUFFERSIZE; samp++) {
		y = cuCmulf(exp_term, y);
		// Always assume input is present -- host will set amplitude to 0 when not.
		// This could be optimized with an "input present" flag given it's usually 0.
		input_complex.x = input_base[samp];
		input_complex.y = 0.0f;
		y = cuCaddf(y, cuCmulf(input_complex, input_amp));

		// Tree-sum, channels interleaved
		float merge_output_L = y.x*pan;
		float merge_output_R = y.x*(1-pan);
		for (int offset = 16; offset > 0; offset /= 2) {
			merge_output_L += __shfl_down_sync(0xffffffff, merge_output_L, offset);
			merge_output_R += __shfl_down_sync(0xffffffff, merge_output_R, offset);
		}
		if (is_first_thread_in_warp) {
			output[whichwarp * (BUFFERSIZE*2) + 2*samp] = merge_output_L;
			output[whichwarp * (BUFFERSIZE * 2) + 2*samp + 1] = merge_output_R;
		}

		// Mono demos:
		/*
		float merge_output = y.x;
		for (int offset = 16; offset > 0; offset /= 2) {
			merge_output += __shfl_down_sync(0xffffffff, merge_output, offset);
		}
		if (is_first_thread_in_warp) {
			output[whichwarp * BUFFERSIZE + 2*samp] = merge_output;
		}
		*/
	}

	// Save state back to shared/global memory for next kernel invocation.
	yprev[2 * i] = y.x;
	yprev[2 * i + 1] = y.y;
}

int main()
{
	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE, // use paging file,
		NULL, // default security
		PAGE_READWRITE,
		0, // max objeect size (high-order)
		SHAREDMEMSIZE,  // max obj size (low-order),
		szName);
	if (hMapFile == nullptr) {
		fprintf(stderr, "shared memory init failed! %d", GetLastError());
		return 1;
	}
	LPCTSTR pBuf = (LPTSTR)MapViewOfFile(hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		SHAREDMEMSIZE);
	if (pBuf == nullptr) {
		fprintf(stderr, "mapviewoffile failed! %d", GetLastError());
		CloseHandle(hMapFile);
		return 1;
	}

	HANDLE hSemaphore = CreateSemaphoreA(NULL, 0, 1, szNameSemaphore);
	if (hSemaphore == nullptr) {
		fprintf(stderr, "could not create semaphore %d", GetLastError());
		CloseHandle(hMapFile);
		return 1;
	}
	HANDLE hSemaphoreGPU = CreateSemaphoreA(NULL, 0, 1, szNameSemaphoreGPU);
	if (hSemaphore == nullptr) {
		fprintf(stderr, "could not create semaphore-gpu %d", GetLastError());
		CloseHandle(hMapFile);
		return 1;
	}

	// Init all our buffers
	cudaError_t cudaStatus;
	cudaStatus = cudaSetDevice(0);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
		return 1;
	}

	float* dev_previousvalues; // previous values of exponential across kernel launches. Interleaved complex.
	int* dev_modeinfo;  // modeinfo, per-drum.
	float* dev_druminfo;  // drum info, per-drum
	float* dev_inputs;  // input signals, per-drum
	float* dev_output_samps;  // output samples, per-warp
	
	cudaStatus = cudaMalloc((void**)&dev_previousvalues, NMODES * 2 * sizeof(float));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc dev_previousvalues failed!");
		return 1;
	}
	cudaStatus = cudaMalloc((void**)&dev_modeinfo, NMODES * sizeof(ModeInfo));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc dev_modeinfo failed!");
		return 1;
	}
	// 8 params per drum
	cudaStatus = cudaMalloc((void**)&dev_druminfo, NDRUMS * sizeof(float) * 8);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc dev_druminfo failed!");
		return 1;
	}
	cudaStatus = cudaMalloc((void**)&dev_inputs, NDRUMS * BUFFERSIZE * sizeof(float));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc dev_inputs failed!");
		return 1;
	}
	cudaStatus = cudaMalloc((void**)&dev_output_samps, NWARPS * 2 * BUFFERSIZE * sizeof(float));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc output_samps failed!");
		return 1;
	}

	int times = 0;
	
	int* sharedmem_modeinfoptr = (int*)pBuf;
	int* sharedmem_druminfoptr = (int*)((char*)sharedmem_modeinfoptr + NMODES * sizeof(ModeInfo));
	int* sharedmem_inputptr = (int*)((char*)sharedmem_druminfoptr + NDRUMS*8*sizeof(float));
	int* sharedmem_outputptr = (int*)((char*)sharedmem_inputptr + NDRUMS * BUFFERSIZE * sizeof(float));
	fprintf(stderr, "gpuaudio kernel process: starting main loop. Ctrl-C to exit.\n");
	while (true) {
		times++;
		WaitForSingleObject(hSemaphore, INFINITE);

		// Copy modes from shared memory to device.
		cudaStatus = cudaMemcpy(dev_modeinfo, sharedmem_modeinfoptr, NMODES * sizeof(ModeInfo), cudaMemcpyHostToDevice);
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaMemcpy modeinfos failed!");
			return 1;
		}
		cudaStatus = cudaMemcpy(dev_druminfo, sharedmem_druminfoptr, NDRUMS * 8*sizeof(float), cudaMemcpyHostToDevice);
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaMemcpy drumInfos failed!");
			return 1;
		}
		cudaStatus = cudaMemcpy(dev_inputs, sharedmem_inputptr, NDRUMS * BUFFERSIZE * sizeof(float), cudaMemcpyHostToDevice);
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaMemcpy inputs failed!");
			return 1;
		}

		// Kernel launch
		// NMODES total. (10 drums * 1024)
		filterbankKernel << <10, 1024>> > (dev_previousvalues, (ModeInfo*)dev_modeinfo, dev_druminfo, dev_inputs, dev_output_samps);

		// Check for any errors launching the kernel
		cudaStatus = cudaGetLastError();
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "Kernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
			return 1;
		}

		// cudaDeviceSynchronize waits for the kernel to finish, and returns
		// any errors encountered during the launch.
		cudaStatus = cudaDeviceSynchronize();
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching kernel!\n", cudaStatus);
			return 1;
		}
		
		// Copy output vector from GPU buffer to host memory.
		cudaStatus = cudaMemcpy(host_samplebuffer, dev_output_samps, BUFFERSIZE*NWARPS*2*sizeof(float), cudaMemcpyDeviceToHost);
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaMemcpy samples-back failed!");
			return 1;
		}

		// Sum up and output to buffer
		float* sampsBuf = ((float*)sharedmem_outputptr);
		for (int samplei = 0; samplei < BUFFERSIZE; samplei++) {
			float sampleL = 0.0f;
			float sampleR = 0.0f;
			for (int j = 0; j < NWARPS; j++) {
				sampleL += host_samplebuffer[j*(BUFFERSIZE*2) + 2*samplei + 0];
				sampleR += host_samplebuffer[j *(BUFFERSIZE*2) + 2 * samplei + 1];
			}
			sampsBuf[2*samplei+0] = sampleL;
			sampsBuf[2*samplei+1] = sampleR;
		}
		ReleaseSemaphore(hSemaphoreGPU, 1, NULL);
	}
    return 0;
}

// Defines a region of posix-style mapped shared memory.
// Also includes semaphores to signal work across processes.

// For this example implementation, this hardcodes magic constants for name and size between
// this code and /gpu/metal/simple-modal
// We also depend on the GPU process to map the memory before we open it, and create the semaphores.

#pragma once

#include <cstddef>
#include <cstring>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>

class MacSharedMemoryRegion {
public:
    // Magic constants to keep in sync with /gpu/metal/simple-modal
    static const size_t kSharedMemSizeBytes = 1024*512*2;
    const char *shared_mem_name = "/drumgpu_shared_memory";
    const char *sem_cpu_name = "/sem_modalfilterbank_cpu";
    const char *sem_gpu_name = "/sem_modalfilterbank_gpu";

    MacSharedMemoryRegion() {} 
    ~MacSharedMemoryRegion() {
        cleanup();
    }

    void init();
    void cleanup();
    bool ready() const { return is_ready; }
    void* getAddr() const { return memory; }
    size_t getSizeBytes() const { return is_ready ? kSharedMemSizeBytes : 0; }

    void signalCPU() {
        if (semCPU) {
            sem_post(semCPU);
        }
    }
    void waitGPU() {
        if (semGPU) {
            sem_wait(semGPU);
        }
    }
private:
    // Whether both shared memory and the semaphores are initialized.
    // Shouldn't need atomic<bool>: should be monotonic and only written during initialization.
    bool is_ready = false;
    
    // Shared memory
    void* memory = nullptr;
    size_t size = 0;

    // Semaphores
    sem_t *semCPU = nullptr;
    sem_t *semGPU = nullptr;
};

void MacSharedMemoryRegion::init() {
    is_ready = false;
    int fd = shm_open(shared_mem_name, O_RDWR, 0666);
    if (fd == -1) {
        // CLEANUP: Consider creating the region here.
        // This would let us start the plugin first.
        // This is a simple technical change but it's likely documented as being owned by the server process in multiple places
        return;
    }

    memory = mmap(NULL, kSharedMemSizeBytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (memory == MAP_FAILED) {
        close(fd);
        return;
    }
    close(fd);

    // Shared memory is ready
    size = kSharedMemSizeBytes;
    is_ready = true;

    // We also need the semaphores.
    semCPU = sem_open(sem_cpu_name, O_RDWR, 0666, 0);
    if (semCPU == SEM_FAILED) {
        cleanup();
        return;
    }
    semGPU = sem_open(sem_gpu_name, O_RDWR, 0666, 0);
    if (semGPU == SEM_FAILED) {
        cleanup();
        return;
    }

    // Success
    is_ready = true;
}

void MacSharedMemoryRegion::cleanup() {
    is_ready = false;

    if (memory) {
        munmap(memory, size);
        memory = nullptr;
    }

    if (semCPU) {
        sem_close(semCPU);
        semCPU = nullptr;
    }

    if (semGPU) {
        sem_close(semGPU);
        semGPU = nullptr;
    }
}

// Whether to show the native controls / debug panel on the left.
// Otherwise the WebUI takes up the entire window.
constexpr bool kShowDebugPanel = false;

// Whether to enable file-based logging.
// This is automatically trimmed, and we don't call it during audio callbacks.
constexpr bool kEnableFileLogging = true;

// Whether to use the local development server rather than the packed WebUI.
constexpr bool kWebUIUseDevServer = false;

// Internal/Debug constants
constexpr int kCymIDOffset = 6;
constexpr bool kLogBufferProcessingTimes = false;

// Whether to force mono for live demos with a floor monitor.
constexpr bool kForceMono = true;


// Constants
// CLEANUP: hardcoded sample rate.
constexpr float kSampleRate = 44100.0f;
constexpr float _hz2rad = 2.0f * 3.141529f / kSampleRate;

constexpr bool kLogLoadedFiles = false;
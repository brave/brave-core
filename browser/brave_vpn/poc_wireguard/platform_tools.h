#ifndef PLATFORM_TOOLS_H_
#define PLATFORM_TOOLS_H_

namespace wireguard {

// Prepares the calling thread to receive shutdown signals. Must be called
// before any other threads are spawned so the signal mask is inherited.
// No-op on platforms that don't need pre-thread-spawn setup (e.g. Windows).
void InitShutdownSignal();

// Blocks the calling thread until the process receives a shutdown signal
// (SIGINT / SIGTERM on POSIX; Ctrl-C / Ctrl-Break / console close on Windows).
// Returns the signal number on POSIX; 0 on Windows.
int WaitForShutdownSignal();

// Returns true if the process is running with elevated privileges
// (uid == 0 on POSIX; Administrator on Windows).
bool IsElevated();

}  // namespace wireguard

#endif  // PLATFORM_TOOLS_H_

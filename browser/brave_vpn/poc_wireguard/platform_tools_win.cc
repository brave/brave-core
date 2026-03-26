#include <windows.h>

#include <shlobj.h>  // IsUserAnAdmin()

#include "brave/browser/brave_vpn/poc_wireguard/platform_tools.h"

namespace wireguard {

namespace {

HANDLE g_shutdown_event = nullptr;

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrl_type) {
  switch (ctrl_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      if (g_shutdown_event) {
        SetEvent(g_shutdown_event);
      }
      // Return TRUE to suppress the default handler (which would terminate
      // the process immediately, skipping our cleanup).
      return TRUE;
    default:
      return FALSE;
  }
}

}  // namespace

void InitShutdownSignal() {
  g_shutdown_event = CreateEventW(nullptr, /*bManualReset=*/TRUE,
                                  /*bInitialState=*/FALSE, nullptr);
  SetConsoleCtrlHandler(ConsoleCtrlHandler, /*Add=*/TRUE);
}

int WaitForShutdownSignal() {
  if (g_shutdown_event) {
    WaitForSingleObject(g_shutdown_event, INFINITE);
    CloseHandle(g_shutdown_event);
    g_shutdown_event = nullptr;
  }
  return 0;
}

bool IsElevated() {
  return !!IsUserAnAdmin();
}

}  // namespace wireguard

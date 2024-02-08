/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_helper/service_main.h"

#include <utility>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"

namespace brave_vpn {
namespace {
// Command line switch "--console" runs the service interactively for
// debugging purposes.
constexpr char kConsoleSwitchName[] = "console";
}  // namespace

ServiceMain* ServiceMain::GetInstance() {
  static base::NoDestructor<ServiceMain> instance;
  return instance.get();
}

bool ServiceMain::InitWithCommandLine(const base::CommandLine* command_line) {
  const base::CommandLine::StringVector args = command_line->GetArgs();
  if (!args.empty()) {
    LOG(ERROR) << "No positional parameters expected.";
    return false;
  }

  // Run interactively if needed.
  if (command_line->HasSwitch(kConsoleSwitchName)) {
    run_routine_ = &ServiceMain::RunInteractive;
  }

  return true;
}

// Start() is the entry point called by WinMain.
int ServiceMain::Start() {
  return (this->*run_routine_)();
}

ServiceMain::ServiceMain()
    : run_routine_(&ServiceMain::RunAsService),
      service_status_handle_(nullptr),
      service_status_() {
  service_status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  service_status_.dwCurrentState = SERVICE_STOPPED;
  service_status_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
}

ServiceMain::~ServiceMain() = default;

int ServiceMain::RunAsService() {
  const std::wstring& service_name(brave_vpn::GetBraveVpnHelperServiceName());
  const SERVICE_TABLE_ENTRY dispatch_table[] = {
      {const_cast<LPTSTR>(service_name.c_str()),
       &ServiceMain::ServiceMainEntry},
      {nullptr, nullptr}};

  if (!::StartServiceCtrlDispatcher(dispatch_table)) {
    service_status_.dwWin32ExitCode = ::GetLastError();
    LOG(ERROR) << "Failed to connect to the service control manager:"
               << service_status_.dwWin32ExitCode;
  }

  return service_status_.dwWin32ExitCode;
}

void ServiceMain::ServiceMainImpl() {
  VLOG(1) << __func__ << " BraveVPN Service started";
  service_status_handle_ = ::RegisterServiceCtrlHandler(
      brave_vpn::GetBraveVpnHelperServiceName().c_str(),
      &ServiceMain::ServiceControlHandler);
  if (service_status_handle_ == nullptr) {
    LOG(ERROR) << "RegisterServiceCtrlHandler failed";
    return;
  }
  SetServiceStatus(SERVICE_RUNNING);
  service_status_.dwWin32ExitCode = ERROR_SUCCESS;
  service_status_.dwCheckPoint = 0;
  service_status_.dwWaitHint = 0;

  // When the Run function returns, the service has stopped.
  const HRESULT hr = Run();
  if (FAILED(hr)) {
    service_status_.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    service_status_.dwServiceSpecificExitCode = hr;
  }

  SetServiceStatus(SERVICE_STOPPED);
}

int ServiceMain::RunInteractive() {
  return Run();
}

// static
void ServiceMain::ServiceControlHandler(DWORD control) {
  ServiceMain* self = ServiceMain::GetInstance();
  switch (control) {
    case SERVICE_CONTROL_STOP:
      self->SignalExit();
      break;

    default:
      break;
  }
}

// static
void WINAPI ServiceMain::ServiceMainEntry(DWORD argc, wchar_t* argv[]) {
  ServiceMain::GetInstance()->ServiceMainImpl();
}

void ServiceMain::SetServiceStatus(DWORD state) {
  ::InterlockedExchange(&service_status_.dwCurrentState, state);
  ::SetServiceStatus(service_status_handle_, &service_status_);
}

HRESULT ServiceMain::Run() {
  VLOG(1) << __func__;
  auto* command_line = base::CommandLine::ForCurrentProcess();
  // Crash itself if crash-me was used.
  if (command_line && command_line->HasSwitch(kBraveVpnHelperCrashMe)) {
    CHECK(!command_line->HasSwitch(kBraveVpnHelperCrashMe))
        << "--crash-me was used.";
  }

  base::SingleThreadTaskExecutor service_task_executor(
      base::MessagePumpType::UI);
  base::RunLoop loop;
  quit_ = loop.QuitClosure();
  dns_handler_.StartVPNConnectionChangeMonitoring();
  loop.Run();
  return S_OK;
}

void ServiceMain::SignalExit() {
  VLOG(1) << __func__;

  std::move(quit_).Run();
}

}  // namespace brave_vpn

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_launcher_factory.h"

#include "base/task/post_task.h"
#include "base/bind.h"
#include "base/process/kill.h"
#include "brave/browser/tor/tor_profile_service_impl.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/service_sandbox_type.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_process_host.h"
#include "content/public/browser/child_process_launcher_utils.h"

using content::BrowserThread;

namespace {
constexpr char kTorProxyScheme[] = "socks5://";
bool g_prevent_tor_launch_for_tests = false;
}

// static
TorLauncherFactory* TorLauncherFactory::GetInstance() {
  return base::Singleton<TorLauncherFactory>::get();
}

TorLauncherFactory::TorLauncherFactory()
    : is_starting_(false),
      tor_pid_(-1),
      control_(tor::TorControl::Create(this)) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (g_prevent_tor_launch_for_tests) {
    tor_pid_ = 1234;
    VLOG(1) << "Skipping the tor process launch in tests.";
    return;
  }

  Init();
}

void TorLauncherFactory::Init() {
  content::ServiceProcessHost::Launch(
      tor_launcher_.BindNewPipeAndPassReceiver(),
      content::ServiceProcessHost::Options()
          .WithDisplayName(IDS_UTILITY_PROCESS_TOR_LAUNCHER_NAME)
          .Pass());

  tor_launcher_.set_disconnect_handler(base::BindOnce(
      &TorLauncherFactory::OnTorLauncherCrashed, base::Unretained(this)));

  tor_launcher_->SetCrashHandler(base::Bind(
                        &TorLauncherFactory::OnTorCrashed,
                        base::Unretained(this)));
}

TorLauncherFactory::~TorLauncherFactory() {}

bool TorLauncherFactory::SetConfig(const tor::TorConfig& config) {
  if (config.empty())
    return false;
  config_ = config;
  return true;
}

void TorLauncherFactory::LaunchTorProcess(const tor::TorConfig& config) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (g_prevent_tor_launch_for_tests) {
    VLOG(1) << "Skipping the tor process launch in tests.";
    return;
  }

  if (is_starting_) {
    LOG(WARNING) << "tor process is already starting";
    return;
  } else {
    is_starting_ = true;
  }

  if (tor_pid_ >= 0) {
    LOG(WARNING) << "tor process(" << tor_pid_ << ") is running";
    return;
  }
  if (!SetConfig(config)) {
    LOG(WARNING) << "config is empty";
    return;
  }

  // Tor launcher could be null if we created Tor process and killed it
  // through KillTorProcess function before. So we need to initialize
  // tor_launcher_ again here.
  if (!tor_launcher_) {
    Init();
  }

  // Launch tor after cleanup is done
  control_->Start(config_.tor_watch_path(),
                  base::BindOnce(&TorLauncherFactory::OnTorControlCheckComplete,
                                 base::Unretained(this)));
}

void TorLauncherFactory::OnTorControlCheckComplete() {
  content::GetUIThreadTaskRunner({})
    ->PostTask(FROM_HERE,
               base::BindOnce(&TorLauncherFactory::Launching,
               base::Unretained(this)));
}

void TorLauncherFactory::Launching() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  tor_launcher_->Launch(config_,
                        base::Bind(&TorLauncherFactory::OnTorLaunched,
                                   base::Unretained(this)));
}

void TorLauncherFactory::ReLaunchTorProcess(const tor::TorConfig& config) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DLOG_IF(ERROR, g_prevent_tor_launch_for_tests)
      << "The tor process launch was suppressed for testing. Feel free to "
         "replace this logging with a condition if you want to prevent "
         "relaunch as well";

  if (is_starting_) {
    LOG(WARNING) << "tor process is already starting";
    return;
  }

  if (tor_pid_ < 0) {
    LaunchTorProcess(config);
    return;
  }
  if (!SetConfig(config)) {
    LOG(WARNING) << "config is empty.";
    return;
  }
  tor_launcher_->ReLaunch(config_,
                        base::Bind(&TorLauncherFactory::OnTorLaunched,
                                   base::Unretained(this)));
}

void TorLauncherFactory::KillTorProcess() {
  if (tor_launcher_.is_bound())
    tor_launcher_->Shutdown();
  control_->Stop();
  tor_launcher_.reset();
  tor_pid_ = -1;
}

void TorLauncherFactory::AddObserver(tor::TorProfileServiceImpl* service) {
  observers_.AddObserver(service);
}

void TorLauncherFactory::RemoveObserver(tor::TorProfileServiceImpl* service) {
  observers_.RemoveObserver(service);
}

void TorLauncherFactory::OnTorLauncherCrashed() {
  LOG(ERROR) << "Tor Launcher Crashed";
  is_starting_ = false;
  for (auto& observer : observers_)
    observer.NotifyTorLauncherCrashed();
}

void TorLauncherFactory::OnTorCrashed(int64_t pid) {
  LOG(ERROR) << "Tor Process(" << pid << ") Crashed";
  is_starting_ = false;
  for (auto& observer : observers_)
    observer.NotifyTorCrashed(pid);
}

void TorLauncherFactory::OnTorLaunched(bool result, int64_t pid) {
  if (result) {
    is_starting_ = false;
    tor_pid_ = pid;
  } else {
    LOG(ERROR) << "Tor Launching Failed(" << pid <<")";
  }
  for (auto& observer : observers_)
    observer.NotifyTorLaunched(result, pid);
}

void TorLauncherFactory::OnTorNewProxyURI(std::string uri) {
  for (auto& observer : observers_)
    observer.NotifyTorNewProxyURI(uri);
}

void TorLauncherFactory::OnTorCircuitEstablished(bool result) {
  for (auto& observer : observers_)
    observer.NotifyTorCircuitEstablished(result);
}

void TorLauncherFactory::OnTorInitializing(std::string percentage) {
  for (auto& observer : observers_)
    observer.NotifyTorInitializing(percentage);
}

void TorLauncherFactory::OnTorControlReady() {
  LOG(ERROR) << "TOR CONTROL: Ready!";
  control_->GetVersion(
      base::BindOnce(&TorLauncherFactory::GotVersion, base::Unretained(this)));
  control_->GetSOCKSListeners(
      base::BindOnce(&TorLauncherFactory::GotSOCKSListeners,
                     base::Unretained(this)));
  control_->Subscribe(tor::TorControlEvent::NETWORK_LIVENESS,
                      base::DoNothing::Once<bool>());
  control_->Subscribe(tor::TorControlEvent::STATUS_CLIENT,
                      base::DoNothing::Once<bool>());
  control_->Subscribe(tor::TorControlEvent::STATUS_GENERAL,
                      base::DoNothing::Once<bool>());
#if 0
  control_->Subscribe(tor::TorControlEvent::STREAM,
                      base::DoNothing::Once<bool>());
#endif
}

void TorLauncherFactory::GotVersion(bool error, const std::string& version) {
  if (error) {
    LOG(ERROR) << "Failed to get version!";
    return;
  }
  LOG(ERROR) << "Tor version: " << version;
}

void TorLauncherFactory::GotSOCKSListeners(
    bool error, const std::vector<std::string>& listeners) {
  if (error) {
    LOG(ERROR) << "Failed to get SOCKS listeners!";
    return;
  }
  LOG(ERROR) << "Tor SOCKS listeners: ";
  for (auto& listener : listeners) {
    LOG(ERROR) << listener;
  }
  std::string tor_proxy_uri = kTorProxyScheme + listeners[0];
  // Remove extra quotes
  tor_proxy_uri.erase(
      std::remove(tor_proxy_uri.begin(), tor_proxy_uri.end(), '\"'),
      tor_proxy_uri.end());
  content::GetUIThreadTaskRunner({})
    ->PostTask(FROM_HERE,
               base::BindOnce(&TorLauncherFactory::OnTorNewProxyURI,
                              base::Unretained(this),
                              std::move(tor_proxy_uri)));
}

void TorLauncherFactory::OnTorClosed() {
  LOG(ERROR) << "TOR CONTROL: Closed!";
}

void TorLauncherFactory::OnTorCleanupNeeded(base::ProcessId id) {
  LOG(ERROR) << "Killing old tor process pid=" << id;
  // Dispatch to launcher thread
  content::GetProcessLauncherTaskRunner()
    ->PostTask(FROM_HERE,
               base::BindOnce(&TorLauncherFactory::KillOldTorProcess,
                              base::Unretained(this),
                              std::move(id)));
}


void TorLauncherFactory::KillOldTorProcess(base::ProcessId id) {
  DCHECK(content::CurrentlyOnProcessLauncherTaskRunner());
  base::Process tor_process = base::Process::Open(id);
  tor_process.Terminate(0, false);
}

void TorLauncherFactory::OnTorEvent(
    tor::TorControlEvent event, const std::string& initial,
    const std::map<std::string, std::string>& extra) {
  LOG(ERROR) << "TOR CONTROL: event "
             << (*tor::kTorControlEventByEnum.find(event)).second
             << ": " << initial;
  if (event == tor::TorControlEvent::STATUS_CLIENT) {
    if (initial.find("BOOTSTRAP") != std::string::npos) {
      const char prefix[] = "PROGRESS=";
      size_t progress_start = initial.find(prefix);
      size_t progress_length = initial.substr(progress_start).find(" ") ;
      // Dispatch progress
      const std::string percentage =
        initial.substr(progress_start + strlen(prefix),
                       progress_length - strlen(prefix));
      content::GetUIThreadTaskRunner({})
        ->PostTask(FROM_HERE,
                   base::BindOnce(&TorLauncherFactory::OnTorInitializing,
                                  base::Unretained(this),
                                  std::move(percentage)));
    } else if (initial.find("CIRCUIT_ESTABLISHED") != std::string::npos) {
      content::GetUIThreadTaskRunner({})
        ->PostTask(FROM_HERE,
                   base::BindOnce(&TorLauncherFactory::OnTorCircuitEstablished,
                                  base::Unretained(this),
                                  true));
    } else if (initial.find("CIRCUIT_NOT_ESTABLISHED") != std::string::npos) {
      content::GetUIThreadTaskRunner({})
        ->PostTask(FROM_HERE,
                   base::BindOnce(&TorLauncherFactory::OnTorCircuitEstablished,
                                  base::Unretained(this),
                                  false));
    }
  }
}

void TorLauncherFactory::OnTorRawCmd(const std::string& cmd) {
  LOG(ERROR) << "TOR CONTROL: command: " << cmd;
}

void TorLauncherFactory::OnTorRawAsync(
    const std::string& status, const std::string& line) {
  LOG(ERROR) << "TOR CONTROL: async " << status << " " << line;
}

void TorLauncherFactory::OnTorRawMid(
    const std::string& status, const std::string& line) {
  LOG(ERROR) << "TOR CONTROL: mid " << status << "-" << line;
}

void TorLauncherFactory::OnTorRawEnd(
    const std::string& status, const std::string& line) {
  LOG(ERROR) << "TOR CONTROL: end " << status << " " << line;
}

ScopedTorLaunchPreventerForTest::ScopedTorLaunchPreventerForTest() {
  g_prevent_tor_launch_for_tests = true;
}

ScopedTorLaunchPreventerForTest::~ScopedTorLaunchPreventerForTest() {
  g_prevent_tor_launch_for_tests = false;
}

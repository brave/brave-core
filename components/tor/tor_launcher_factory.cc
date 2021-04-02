/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_launcher_factory.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_post_task.h"
#include "base/files/file_util.h"
#include "base/sequenced_task_runner.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/tor/service_sandbox_type.h"
#include "brave/components/tor/tor_file_watcher.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_process_host.h"

using content::BrowserThread;

namespace {
constexpr char kTorProxyScheme[] = "socks5://";
// tor::TorControlEvent::STATUS_CLIENT response
constexpr char kStatusClientBootstrap[] = "BOOTSTRAP";
constexpr char kStatusClientBootstrapProgress[] = "PROGRESS=";
constexpr char kStatusClientCircuitEstablished[] = "CIRCUIT_ESTABLISHED";
constexpr char kStatusClientCircuitNotEstablished[] = "CIRCUIT_NOT_ESTABLISHED";

std::pair<bool, std::string> LoadTorLogOnFileTaskRunner(
    const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);
  std::pair<bool, std::string> result;
  result.first = success;
  if (success) {
    result.second = data;
  }
  return result;
}
}  // namespace

// static
TorLauncherFactory* TorLauncherFactory::GetInstance() {
  return base::Singleton<TorLauncherFactory>::get();
}

TorLauncherFactory::TorLauncherFactory()
    : is_starting_(false),
      is_connected_(false),
      tor_pid_(-1),
      control_(new tor::TorControl(this->AsWeakPtr(),
                                   content::GetIOThreadTaskRunner({})),
               base::OnTaskRunnerDeleter(content::GetIOThreadTaskRunner({}))),
      weak_ptr_factory_(this) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void TorLauncherFactory::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  content::ServiceProcessHost::Launch(
      tor_launcher_.BindNewPipeAndPassReceiver(),
      content::ServiceProcessHost::Options()
          .WithDisplayName(IDS_UTILITY_PROCESS_TOR_LAUNCHER_NAME)
          .Pass());

  tor_launcher_.set_disconnect_handler(
      base::BindOnce(&TorLauncherFactory::OnTorLauncherCrashed,
                     weak_ptr_factory_.GetWeakPtr()));

  tor_launcher_->SetCrashHandler(base::BindOnce(
      &TorLauncherFactory::OnTorCrashed, weak_ptr_factory_.GetWeakPtr()));
}

TorLauncherFactory::~TorLauncherFactory() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void TorLauncherFactory::LaunchTorProcess(const tor::mojom::TorConfig& config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

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

  DCHECK(!config.binary_path.empty());
  DCHECK(!config.tor_data_path.empty());
  DCHECK(!config.tor_watch_path.empty());
  config_ = config;

  // Tor launcher could be null if we created Tor process and killed it
  // through KillTorProcess function before. So we need to initialize
  // tor_launcher_ again here.
  if (!tor_launcher_) {
    Init();
  }

  LaunchTorInternal();
}

void TorLauncherFactory::OnTorLogLoaded(
    GetLogCallback callback,
    const std::pair<bool, std::string>& result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(result.first, result.second);
}

void TorLauncherFactory::LaunchTorInternal() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (tor_launcher_.is_bound()) {
    auto config = tor::mojom::TorConfig::New(config_);
    tor_launcher_->Launch(std::move(config),
                          base::BindOnce(&TorLauncherFactory::OnTorLaunched,
                                         weak_ptr_factory_.GetWeakPtr()));
  } else {
    is_starting_ = false;
  }
}

void TorLauncherFactory::KillTorProcess() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (tor_launcher_.is_bound())
    tor_launcher_->Shutdown();
  control_->Stop();
  tor_launcher_.reset();
  tor_pid_ = -1;
  is_connected_ = false;
}

int64_t TorLauncherFactory::GetTorPid() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return tor_pid_;
}

bool TorLauncherFactory::IsTorConnected() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return is_connected_;
}

std::string TorLauncherFactory::GetTorProxyURI() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return tor_proxy_uri_;
}

std::string TorLauncherFactory::GetTorVersion() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return tor_version_;
}

void TorLauncherFactory::GetTorLog(GetLogCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::FilePath tor_log_path = config_.tor_data_path.AppendASCII("tor.log");
  scoped_refptr<base::SequencedTaskRunner> file_task_runner(
      base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN}));
  base::PostTaskAndReplyWithResult(
      file_task_runner.get(), FROM_HERE,
      base::BindOnce(&LoadTorLogOnFileTaskRunner, tor_log_path),
      base::BindOnce(&TorLauncherFactory::OnTorLogLoaded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void TorLauncherFactory::AddObserver(TorLauncherObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void TorLauncherFactory::RemoveObserver(TorLauncherObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void TorLauncherFactory::OnTorLauncherCrashed() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LOG(INFO) << "Tor Launcher Crashed";
  for (auto& observer : observers_)
    observer.OnTorLauncherCrashed();
  DelayedRelaunchTor();
}

void TorLauncherFactory::OnTorCrashed(int64_t pid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LOG(INFO) << "Tor Process(" << pid << ") Crashed";
  for (auto& observer : observers_)
    observer.OnTorCrashed(pid);
  DelayedRelaunchTor();
}

void TorLauncherFactory::OnTorLaunched(bool result, int64_t pid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (auto& observer : observers_)
    observer.OnTorLaunched(result, pid);
  if (result) {
    is_starting_ = false;
    // We have to wait for circuit established
    is_connected_ = false;
    tor_pid_ = pid;
  } else {
    LOG(ERROR) << "Tor Launching Failed(" << pid << ")";
    return;
  }

  tor::TorFileWatcher* tor_file_watcher =
      new tor::TorFileWatcher(config_.tor_watch_path);
  tor_file_watcher->StartWatching(base::BindPostTask(
      base::SequencedTaskRunnerHandle::Get(),
      base::BindOnce(&TorLauncherFactory::OnTorControlPrerequisitesReady,
                     weak_ptr_factory_.GetWeakPtr(), pid)));
}

void TorLauncherFactory::OnTorControlReady() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << "TOR CONTROL: Ready!";
  control_->GetVersion(
      base::BindPostTask(base::SequencedTaskRunnerHandle::Get(),
                         base::BindOnce(&TorLauncherFactory::GotVersion,
                                        weak_ptr_factory_.GetWeakPtr())));
  control_->GetSOCKSListeners(
      base::BindPostTask(base::SequencedTaskRunnerHandle::Get(),
                         base::BindOnce(&TorLauncherFactory::GotSOCKSListeners,
                                        weak_ptr_factory_.GetWeakPtr())));
  // A Circuit might have been established when Tor control is ready, in that
  // case we will not receive circuit established events. So we query the status
  // directly as fail safe, otherwise Tor window might stuck in disconnected
  // state while Tor circuit is ready.
  control_->GetCircuitEstablished(base::BindPostTask(
      base::SequencedTaskRunnerHandle::Get(),
      base::BindOnce(&TorLauncherFactory::GotCircuitEstablished,
                     weak_ptr_factory_.GetWeakPtr())));
  control_->Subscribe(tor::TorControlEvent::NETWORK_LIVENESS,
                      base::DoNothing::Once<bool>());
  control_->Subscribe(tor::TorControlEvent::STATUS_CLIENT,
                      base::DoNothing::Once<bool>());
  control_->Subscribe(tor::TorControlEvent::STATUS_GENERAL,
                      base::DoNothing::Once<bool>());
  control_->Subscribe(tor::TorControlEvent::STREAM,
                      base::DoNothing::Once<bool>());
}

void TorLauncherFactory::GotVersion(bool error, const std::string& version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (error) {
    VLOG(1) << "Failed to get version!";
    return;
  }
  VLOG(2) << "Tor version: " << version;
  tor_version_ = version;
}

void TorLauncherFactory::GotSOCKSListeners(
    bool error,
    const std::vector<std::string>& listeners) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (error) {
    VLOG(1) << "Failed to get SOCKS listeners!";
    return;
  }
  if (VLOG_IS_ON(2)) {
    VLOG(2) << "Tor SOCKS listeners: ";
    for (auto& listener : listeners) {
      VLOG(2) << listener;
    }
  }
  std::string tor_proxy_uri = kTorProxyScheme + listeners[0];
  // Remove extra quotes
  tor_proxy_uri.erase(
      std::remove(tor_proxy_uri.begin(), tor_proxy_uri.end(), '\"'),
      tor_proxy_uri.end());
  tor_proxy_uri_ = tor_proxy_uri;
  for (auto& observer : observers_)
    observer.OnTorNewProxyURI(tor_proxy_uri);
}

void TorLauncherFactory::GotCircuitEstablished(bool error, bool established) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (error) {
    VLOG(1) << "Failed to get circuit established!";
    return;
  }
  is_connected_ = established;
  for (auto& observer : observers_)
    observer.OnTorCircuitEstablished(established);
}

void TorLauncherFactory::OnTorControlClosed(bool was_running) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << "TOR CONTROL: Closed!";
  // If we're still running, try watching again to start over.
  // TODO(riastradh-brave): Rate limit in case of flapping?
  if (was_running) {
    LaunchTorInternal();
  }
}

void TorLauncherFactory::OnTorControlPrerequisitesReady(
    int64_t pid,
    bool ready,
    std::vector<uint8_t> cookie,
    int port) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (pid != tor_pid_) {
    VLOG(1) << "Tor control pid mismatched!";
    return;
  }
  if (ready) {
    control_->Start(std::move(cookie), port);
  } else {
    tor::TorFileWatcher* tor_file_watcher =
        new tor::TorFileWatcher(config_.tor_watch_path);
    tor_file_watcher->StartWatching(base::BindPostTask(
        base::SequencedTaskRunnerHandle::Get(),
        base::BindOnce(&TorLauncherFactory::OnTorControlPrerequisitesReady,
                       weak_ptr_factory_.GetWeakPtr(), pid)));
  }
}

void TorLauncherFactory::RelaunchTor() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  Init();
  LaunchTorInternal();
}

void TorLauncherFactory::DelayedRelaunchTor() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  is_starting_ = false;
  is_connected_ = false;
  KillTorProcess();
  // Post delayed relaunch for control to stop
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&TorLauncherFactory::RelaunchTor,
                     weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(1));
}

void TorLauncherFactory::OnTorEvent(
    tor::TorControlEvent event,
    const std::string& initial,
    const std::map<std::string, std::string>& extra) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const std::string raw_event =
      (*tor::kTorControlEventByEnum.find(event)).second + ": " + initial;
  VLOG(3) << "TOR CONTROL: event " << raw_event;
  for (auto& observer : observers_)
    observer.OnTorControlEvent(raw_event);
  if (event == tor::TorControlEvent::STATUS_CLIENT) {
    if (initial.find(kStatusClientBootstrap) != std::string::npos) {
      size_t progress_start = initial.find(kStatusClientBootstrapProgress);
      size_t progress_length = initial.substr(progress_start).find(" ");
      // Dispatch progress
      const std::string percentage = initial.substr(
          progress_start + strlen(kStatusClientBootstrapProgress),
          progress_length - strlen(kStatusClientBootstrapProgress));
      for (auto& observer : observers_)
        observer.OnTorInitializing(percentage);
    } else if (initial.find(kStatusClientCircuitEstablished) !=
               std::string::npos) {
      for (auto& observer : observers_)
        observer.OnTorCircuitEstablished(true);
      is_connected_ = true;
    } else if (initial.find(kStatusClientCircuitNotEstablished) !=
               std::string::npos) {
      for (auto& observer : observers_)
        observer.OnTorCircuitEstablished(false);
    }
  }
}

void TorLauncherFactory::OnTorRawCmd(const std::string& cmd) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(3) << "TOR CONTROL: command: " << cmd;
}

void TorLauncherFactory::OnTorRawAsync(const std::string& status,
                                       const std::string& line) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(3) << "TOR CONTROL: async " << status << " " << line;
}

void TorLauncherFactory::OnTorRawMid(const std::string& status,
                                     const std::string& line) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(3) << "TOR CONTROL: mid " << status << "-" << line;
}

void TorLauncherFactory::OnTorRawEnd(const std::string& status,
                                     const std::string& line) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(3) << "TOR CONTROL: end " << status << " " << line;
}

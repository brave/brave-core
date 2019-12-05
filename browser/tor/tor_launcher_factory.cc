/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_launcher_factory.h"

#include "brave/browser/tor/tor_profile_service_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

using content::BrowserThread;

namespace {
bool g_prevent_tor_launch_for_tests = false;
}

// static
TorLauncherFactory* TorLauncherFactory::GetInstance() {
  return base::Singleton<TorLauncherFactory>::get();
}

TorLauncherFactory::TorLauncherFactory()
    : is_starting_(false),
      tor_pid_(-1) {
  if (g_prevent_tor_launch_for_tests) {
    tor_pid_ = 1234;
    VLOG(1) << "Skipping the tor process launch in tests.";
    return;
  }

  Init();
}

void TorLauncherFactory::Init() {
  content::ServiceManagerConnection::GetForProcess()->GetConnector()->Connect(
      tor::mojom::kServiceName, tor_launcher_.BindNewPipeAndPassReceiver());

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

ScopedTorLaunchPreventerForTest::ScopedTorLaunchPreventerForTest() {
  g_prevent_tor_launch_for_tests = true;
}

ScopedTorLaunchPreventerForTest::~ScopedTorLaunchPreventerForTest() {
  g_prevent_tor_launch_for_tests = false;
}

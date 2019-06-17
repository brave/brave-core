/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/mock_tor_profile_service_impl.h"

#include <string>

#include "base/callback.h"
#include "brave/common/tor/tor_test_constants.h"

namespace tor {

MockTorProfileServiceImpl::MockTorProfileServiceImpl() {
  base::FilePath path(kTestTorPath);
  std::string proxy(kTestTorProxy);
  config_ = TorConfig(path, proxy);
}

MockTorProfileServiceImpl::~MockTorProfileServiceImpl() {}

void MockTorProfileServiceImpl::LaunchTor(const TorConfig& config) {}

void MockTorProfileServiceImpl::ReLaunchTor(const TorConfig& config) {
  config_ = config;
}

void MockTorProfileServiceImpl::SetNewTorCircuit(
    const GURL& request_url,
    NewTorCircuitCallback callback) {}

const TorConfig& MockTorProfileServiceImpl::GetTorConfig() {
  return config_;
}

int64_t MockTorProfileServiceImpl::GetTorPid() { return -1; }

}  // namespace tor

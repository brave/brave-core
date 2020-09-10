/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/tls_deprecation_config_component_installer.h"

#define ReconfigureAfterNetworkRestart \
  ReconfigureAfterNetworkRestart_ChromiumImpl
#include "../../../../../chrome/browser/component_updater/tls_deprecation_config_component_installer.cc"  // NOLINT
#undef ReconfigureAfterNetworkRestart

#include "base/bind.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_thread.h"
#include "services/network/public/proto/tls_deprecation_config.pb.h"

namespace component_updater {

namespace {

std::string LoadEmptyConfig() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  auto config =
      std::make_unique<chrome_browser_ssl::LegacyTLSExperimentConfig>();
  config->set_version_id(1);
  return config->SerializeAsString();
}

}  // namespace

// static
void TLSDeprecationConfigComponentInstallerPolicy::
    ReconfigureAfterNetworkRestart() {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_VISIBLE, base::MayBlock()},
      base::BindOnce(&LoadEmptyConfig),
      base::BindOnce(&UpdateLegacyTLSConfigOnUI));
}

}  // namespace component_updater

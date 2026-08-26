/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"

#include <memory>
#include <optional>
#include <utility>

#include "brave/browser/widevine/widevine_utils.h"

#define RegisterWidevineCdmComponent RegisterWidevineCdmComponent_ChromiumImpl
#include <chrome/browser/component_updater/widevine_cdm_component_installer.cc>
#undef RegisterWidevineCdmComponent

namespace component_updater {

namespace {

class BraveWidevineCdmComponentInstallerPolicy
    : public WidevineCdmComponentInstallerPolicy {
 private:
  // Upstream returns false because the CRX payload is signature verified.
  // Require TLS anyway so the request doesn't advertise in plaintext that this
  // user is installing Widevine.
  bool RequiresNetworkEncryption() const override { return true; }
};

}  // namespace

void RegisterWidevineCdmComponent(ComponentUpdateService* cus,
                                  base::OnceClosure callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!IsWidevineEnabled()) {
    return;
  }
  auto installer = base::MakeRefCounted<ComponentInstaller>(
      std::make_unique<BraveWidevineCdmComponentInstallerPolicy>());
  installer->Register(cus, std::move(callback));
}

}  // namespace component_updater

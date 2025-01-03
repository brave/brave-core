/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_BRAVE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_BRAVE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service_delegate.h"
#include "components/content_settings/core/browser/cookie_settings.h"

namespace content {
class BrowserContext;
}

class HostContentSettingsMap;

namespace ephemeral_storage {

class BraveEphemeralStorageServiceDelegate
    : public EphemeralStorageServiceDelegate {
 public:
  BraveEphemeralStorageServiceDelegate(
      content::BrowserContext* context,
      HostContentSettingsMap* host_content_settings_map,
      scoped_refptr<content_settings::CookieSettings> cookie_settings);
  ~BraveEphemeralStorageServiceDelegate() override;

  void CleanupTLDEphemeralArea(const TLDEphemeralAreaKey& key) override;
  void CleanupFirstPartyStorageArea(
      const std::string& registerable_domain) override;
  bool DoesProfileHaveAnyBrowserWindow() const override;

 private:
  raw_ptr<content::BrowserContext> context_ = nullptr;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_BRAVE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_

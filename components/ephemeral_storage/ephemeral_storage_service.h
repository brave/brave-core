/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class HostContentSettingsMap;

namespace content {
class BrowserContext;
class StoragePartition;
}  // namespace content

namespace ephemeral_storage {

// Service to enable or disable first party ephemeral storage from external
// actors.
class EphemeralStorageService
    : public KeyedService,
      public base::SupportsWeakPtr<EphemeralStorageService> {
 public:
  EphemeralStorageService(content::BrowserContext* context,
                          HostContentSettingsMap* host_content_settings_map);
  ~EphemeralStorageService() override;

  // Performs storage check (cookies, localStorage) and callbacks `true` if
  // nothing is stored in all of these storages.
  void CanEnable1PESForUrl(
      const GURL& url,
      base::OnceCallback<void(bool can_enable_1pes)> callback) const;
  // Enables/disables first party ephemeral storage for |url|.
  void Set1PESEnabledForUrl(const GURL& url, bool enable);
  // Returns current state of first party ephemeral storage mode for |url|.
  bool Is1PESEnabledForUrl(const GURL& url) const;

 private:
  content::BrowserContext* context_ = nullptr;
  HostContentSettingsMap* host_content_settings_map_ = nullptr;
};

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_H_

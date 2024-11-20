/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_H_

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/unguessable_token.h"
#include "base/values.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service_delegate.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service_observer.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/storage_partition_config.h"
#include "url/gurl.h"
#include "url/origin.h"

class EphemeralStorageBrowserTest;
class EphemeralStorageQaBrowserTest;
class HostContentSettingsMap;
class PrefService;

namespace content {
class BrowserContext;
}  // namespace content

namespace permissions {
class PermissionLifetimeManagerBrowserTest;
}

namespace ephemeral_storage {

// Handles Ephemeral Storage cleanup/queuing and other events.
class EphemeralStorageService : public KeyedService {
 public:
  EphemeralStorageService(
      content::BrowserContext* context,
      HostContentSettingsMap* host_content_settings_map,
      std::unique_ptr<EphemeralStorageServiceDelegate> delegate);
  ~EphemeralStorageService() override;

  void Shutdown() override;

  base::WeakPtr<EphemeralStorageService> GetWeakPtr();

  // Performs storage check (cookies, localStorage) and callbacks `true` if
  // nothing is stored in all of these storages.
  void CanEnable1PESForUrl(
      const GURL& url,
      base::OnceCallback<void(bool can_enable_1pes)> callback) const;
  // Enables/disables first party ephemeral storage for |url|.
  void Set1PESEnabledForUrl(const GURL& url, bool enable);
  // Returns current state of first party ephemeral storage mode for |url|.
  bool Is1PESEnabledForUrl(const GURL& url) const;
  // Enables 1PES for url if nothing is stored for |url|.
  void Enable1PESForUrlIfPossible(const GURL& url,
                                  base::OnceCallback<void(bool)> on_ready);
  // Returns First Party Ephemeral Storage token to partition storage.
  std::optional<base::UnguessableToken> Get1PESToken(const url::Origin& origin);

  void TLDEphemeralLifetimeCreated(
      const std::string& ephemeral_domain,
      const content::StoragePartitionConfig& storage_partition_config);
  void TLDEphemeralLifetimeDestroyed(
      const std::string& ephemeral_domain,
      const content::StoragePartitionConfig& storage_partition_config,
      bool shields_disabled_on_one_of_hosts);

  void AddObserver(EphemeralStorageServiceObserver* observer);
  void RemoveObserver(EphemeralStorageServiceObserver* observer);

 private:
  friend EphemeralStorageBrowserTest;
  friend EphemeralStorageQaBrowserTest;
  friend permissions::PermissionLifetimeManagerBrowserTest;

  void FirstPartyStorageAreaInUse(const std::string& ephemeral_domain);
  bool FirstPartyStorageAreaNotInUse(const std::string& ephemeral_domain,
                                     bool shields_disabled_on_one_of_hosts);

  void OnCanEnable1PESForUrl(const GURL& url,
                             base::OnceCallback<void(bool)> on_ready,
                             bool can_enable_1pes);
  bool IsDefaultCookieSetting(const GURL& url) const;

  void CleanupTLDEphemeralAreaByTimer(const TLDEphemeralAreaKey& key,
                                      bool cleanup_tld_ephemeral_area,
                                      bool cleanup_first_party_storage_area);
  void CleanupTLDEphemeralArea(const TLDEphemeralAreaKey& key,
                               bool cleanup_tld_ephemeral_area,
                               bool cleanup_first_party_storage_area);

  // If a website was closed, but not yet cleaned-up because of storage lifetime
  // keepalive, we store the origin into a pref to perform a cleanup on browser
  // startup. It's impossible to do a cleanup on shutdown, because the process
  // is asynchronous and cannot block the browser shutdown.
  void ScheduleFirstPartyStorageAreasCleanupOnStartup();
  void CleanupFirstPartyStorageAreasOnStartup();
  void CleanupFirstPartyStorageArea(const std::string& ephemeral_domain);

  size_t FireCleanupTimersForTesting();

  raw_ptr<content::BrowserContext> context_ = nullptr;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  std::unique_ptr<EphemeralStorageServiceDelegate> delegate_;
  raw_ptr<PrefService> prefs_ = nullptr;
  // These patterns are removed on service Shutdown.
  base::flat_set<ContentSettingsPattern> patterns_to_cleanup_on_shutdown_;

  base::ObserverList<EphemeralStorageServiceObserver> observer_list_;

  base::TimeDelta tld_ephemeral_area_keep_alive_;
  base::TimeDelta first_party_storage_startup_cleanup_delay_;
  std::map<TLDEphemeralAreaKey, std::unique_ptr<base::OneShotTimer>>
      tld_ephemeral_areas_to_cleanup_;
  // Contains First Party Ephemeral Storage tokens to partition storage.
  base::flat_map<std::string, base::UnguessableToken> fpes_tokens_;
  base::Value::List first_party_storage_areas_to_cleanup_on_startup_;
  base::OneShotTimer first_party_storage_areas_startup_cleanup_timer_;

  base::WeakPtrFactory<EphemeralStorageService> weak_ptr_factory_{this};
};

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_H_

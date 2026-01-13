/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_BRAVE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_BRAVE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_

#include <memory>

#include "base/containers/flat_set.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/ephemeral_storage/application_state_observer.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service_delegate.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "content/public/browser/web_contents_delegate.h"

namespace content {
class BrowserContext;
}

class HostContentSettingsMap;

namespace ephemeral_storage {

class BraveEphemeralStorageServiceDelegate
    : public ApplicationStateObserver::Observer,
      public EphemeralStorageServiceDelegate {
 public:
  BraveEphemeralStorageServiceDelegate(
      content::BrowserContext* context,
      HostContentSettingsMap* host_content_settings_map,
      scoped_refptr<content_settings::CookieSettings> cookie_settings,
      brave_shields::BraveShieldsSettingsService* shields_settings_service);
  ~BraveEphemeralStorageServiceDelegate() override;

  // ApplicationStateObserver::Observer:
  void OnApplicationBecameActive() override;
  void OnApplicationBecameInactive() override;

  // EphemeralStorageServiceDelegate:
  void CleanupTLDEphemeralArea(const TLDEphemeralAreaKey& key) override;
  void CleanupFirstPartyStorageArea(const TLDEphemeralAreaKey& key) override;
  void RegisterFirstWindowOpenedCallback(base::OnceClosure callback) override;
  void RegisterOnBecomeActiveCallback(
      base::OnceCallback<void(const base::flat_set<std::string>)> callback)
      override;
  void PrepareTabsForFirstPartyStorageCleanup(
      const std::vector<std::string>& ephemeral_domains) override;
  bool IsShieldsDisabledOnAnyHostMatchingDomainOf(
      const GURL& url) const override;
#if BUILDFLAG(IS_ANDROID)
  void TriggerCurrentAppStateNotification() override;
#endif
 private:
  raw_ptr<content::BrowserContext> context_ = nullptr;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
  base::OnceClosure first_window_opened_callback_;
  base::OnceCallback<void(const base::flat_set<std::string>)>
      on_become_active_callback_;
  std::unique_ptr<ApplicationStateObserver> application_state_observer_;
  raw_ptr<brave_shields::BraveShieldsSettingsService>
      shields_settings_service_ = nullptr;
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_BRAVE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_

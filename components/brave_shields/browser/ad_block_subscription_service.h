/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "base/util/timer/wall_clock_timer.h"
#include "brave/components/brave_shields/browser/ad_block_base_service.h"

class AdBlockServiceTest;

namespace brave_shields {

struct FilterListSubscriptionInfo {
  // The URL used to fetch the list, which is also used as a unique identifier
  // for a subscription service.
  GURL list_url;

  // These are null (by JS representation) if no download has been
  // attempted/succeeded. If a subscription has been successfully downloaded,
  // both of these are exactly equal.
  base::Time last_update_attempt;
  base::Time last_successful_update_attempt;

  // Any enabled list will be queried during network requests and page loads,
  // otherwise it will be bypassed. Disabled lists will not be automatically
  // updated.
  bool enabled;
};

FilterListSubscriptionInfo BuildInfoFromDict(const GURL& list_url,
                                             const base::Value* dict);

// The brave shields service in charge of ad-block checking and init
// for a custom filter list subscription.
class AdBlockSubscriptionService : public AdBlockBaseService {
 public:
  using RefreshSubscriptionCallback = base::RepeatingCallback<void()>;

  // Constructor for a new subscription
  explicit AdBlockSubscriptionService(
      const GURL& list_url,
      RefreshSubscriptionCallback refresh_callback,
      brave_component_updater::BraveComponent::Delegate* delegate);
  // Constructor from cached information
  explicit AdBlockSubscriptionService(
      const FilterListSubscriptionInfo& cached_info,
      RefreshSubscriptionCallback refresh_callback,
      brave_component_updater::BraveComponent::Delegate* delegate);
  ~AdBlockSubscriptionService() override;

  FilterListSubscriptionInfo GetInfo() const;
  std::string GetText() const { return list_contents_; }
  void SetEnabled(bool enabled);

  void OnSuccessfulDownload();
  void OnUnsuccessfulDownload();

 protected:
  bool Init() override;
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::AdBlockServiceTest;
  static std::string g_ad_block_regional_component_id_;
  static std::string g_ad_block_regional_component_base64_public_key_;
  static std::string g_ad_block_regional_dat_file_version_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);
  void ReloadFilters();
  void ScheduleRefreshOnUIThread(base::Time next_download_time);

  RefreshSubscriptionCallback refresh_callback_;

  std::string list_contents_;

  std::string component_id_;
  std::string base64_public_key_;

  GURL list_url_;
  bool enabled_;
  base::Time last_update_attempt_;
  base::Time last_successful_update_attempt_;

  util::WallClockTimer update_timer_;

  base::WeakPtrFactory<AdBlockSubscriptionService> weak_factory_{this};

  AdBlockSubscriptionService(const AdBlockSubscriptionService&) = delete;
  AdBlockSubscriptionService& operator=(const AdBlockSubscriptionService&) =
      delete;
};

// Creates the AdBlockSubscriptionService
std::unique_ptr<AdBlockSubscriptionService> AdBlockSubscriptionServiceFactory(
    const GURL& list_url,
    AdBlockSubscriptionService::RefreshSubscriptionCallback refresh_callback,
    brave_component_updater::BraveComponent::Delegate* delegate);

std::unique_ptr<AdBlockSubscriptionService> AdBlockSubscriptionServiceFactory(
    const FilterListSubscriptionInfo& info,
    AdBlockSubscriptionService::RefreshSubscriptionCallback refresh_callback,
    brave_component_updater::BraveComponent::Delegate* delegate);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_H_

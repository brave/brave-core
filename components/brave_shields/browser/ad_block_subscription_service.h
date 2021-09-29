/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "brave/components/brave_shields/browser/ad_block_base_service.h"
#include "url/gurl.h"

namespace base {
template <typename StructType>
class JSONValueConverter;
}

class AdBlockServiceTest;

namespace brave_shields {

struct SubscriptionInfo {
  // The URL used to fetch the list, which is also used as a unique identifier
  // for a subscription service.
  GURL subscription_url;

  // These are base::Time::Min() if no download has been
  // attempted/succeeded. If a subscription has been successfully downloaded,
  // both of these are exactly equal.
  base::Time last_update_attempt;
  base::Time last_successful_update_attempt;

  // Any enabled list will be queried during network requests and page loads,
  // otherwise it will be bypassed. Disabled lists will not be automatically
  // updated.
  bool enabled;

  static void RegisterJSONConverter(
      base::JSONValueConverter<SubscriptionInfo>*);
};

// The brave shields service in charge of ad-block checking and init
// for a custom filter list subscription.
class AdBlockSubscriptionService : public AdBlockBaseService {
 public:
  using OnLoadCallback = base::RepeatingCallback<void(const GURL&)>;
  explicit AdBlockSubscriptionService(
      const SubscriptionInfo& info,
      base::FilePath list_file,
      brave_component_updater::BraveComponent::Delegate* delegate);
  ~AdBlockSubscriptionService() override;

  void ReloadList();

  bool Init() override;

 private:
  friend class ::AdBlockServiceTest;

  void OnListLoaded();

  GURL subscription_url_;
  base::FilePath list_file_;
  bool load_on_start_;
  bool initialized_;

  base::WeakPtrFactory<AdBlockSubscriptionService> weak_factory_{this};

  AdBlockSubscriptionService(const AdBlockSubscriptionService&) = delete;
  AdBlockSubscriptionService& operator=(const AdBlockSubscriptionService&) =
      delete;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_H_

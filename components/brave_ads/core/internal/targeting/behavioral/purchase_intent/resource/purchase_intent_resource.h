/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_RESOURCE_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_parsing_error_or.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource_info.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

class PurchaseIntentResource final : public AdsClientNotifierObserver {
 public:
  PurchaseIntentResource();

  PurchaseIntentResource(const PurchaseIntentResource&) = delete;
  PurchaseIntentResource& operator=(const PurchaseIntentResource&) = delete;

  PurchaseIntentResource(PurchaseIntentResource&&) noexcept = delete;
  PurchaseIntentResource& operator=(PurchaseIntentResource&&) noexcept = delete;

  ~PurchaseIntentResource() override;

  bool IsLoaded() const { return !!resource_; }

  std::optional<std::string> GetManifestVersion() const {
    return manifest_version_;
  }

  const std::optional<PurchaseIntentResourceInfo>& get() const {
    return resource_;
  }

 private:
  void MaybeLoad();
  void MaybeLoadOrUnload();

  void Load();
  void LoadCallback(
      ResourceComponentParsingErrorOr<PurchaseIntentResourceInfo> result);

  void MaybeUnload();
  void Unload();

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyResourceComponentDidChange(const std::string& manifest_version,
                                          const std::string& id) override;
  void OnNotifyDidUnregisterResourceComponent(const std::string& id) override;

  std::optional<std::string> manifest_version_;

  std::optional<PurchaseIntentResourceInfo> resource_;

  base::WeakPtrFactory<PurchaseIntentResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_RESOURCE_H_

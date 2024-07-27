/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_ANTI_TARGETING_RESOURCE_ANTI_TARGETING_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_ANTI_TARGETING_RESOURCE_ANTI_TARGETING_RESOURCE_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_parsing_error_or.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource_info.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

class AntiTargetingResource final : public AdsClientNotifierObserver {
 public:
  AntiTargetingResource();

  AntiTargetingResource(const AntiTargetingResource&) = delete;
  AntiTargetingResource& operator=(const AntiTargetingResource&) = delete;

  AntiTargetingResource(AntiTargetingResource&&) noexcept = delete;
  AntiTargetingResource& operator=(AntiTargetingResource&&) noexcept = delete;

  ~AntiTargetingResource() override;

  bool IsLoaded() const { return !!resource_; }

  std::optional<std::string> GetManifestVersion() const {
    return manifest_version_;
  }

  AntiTargetingSiteList GetSites(const std::string& creative_set_id) const;

 private:
  void MaybeLoad();
  void MaybeLoadOrUnload();

  void Load();
  void LoadCallback(
      ResourceComponentParsingErrorOr<AntiTargetingResourceInfo> result);

  void MaybeUnload();
  void Unload();

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyResourceComponentDidChange(const std::string& manifest_version,
                                          const std::string& id) override;
  void OnNotifyDidUnregisterResourceComponent(const std::string& id) override;

  std::optional<std::string> manifest_version_;

  std::optional<AntiTargetingResourceInfo> resource_;

  base::WeakPtrFactory<AntiTargetingResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_ANTI_TARGETING_RESOURCE_ANTI_TARGETING_RESOURCE_H_

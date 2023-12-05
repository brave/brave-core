/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_RESOURCE_CONVERSION_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_RESOURCE_CONVERSION_RESOURCE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_parsing_error_or.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_info.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

class ConversionResource final : public AdsClientNotifierObserver {
 public:
  ConversionResource();

  ConversionResource(const ConversionResource&) = delete;
  ConversionResource& operator=(const ConversionResource&) = delete;

  ConversionResource(ConversionResource&&) noexcept = delete;
  ConversionResource& operator=(ConversionResource&&) noexcept = delete;

  ~ConversionResource() override;

  bool IsInitialized() const { return is_initialized_; }

  const ConversionResourceInfo& get() const { return conversion_resource_; }

 private:
  void Load();
  void LoadCallback(ResourceParsingErrorOr<ConversionResourceInfo> result);

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyDidUpdateResourceComponent(const std::string& manifest_version,
                                          const std::string& id) override;

  bool is_initialized_ = false;

  ConversionResourceInfo conversion_resource_;

  base::WeakPtrFactory<ConversionResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_RESOURCE_CONVERSION_RESOURCE_H_

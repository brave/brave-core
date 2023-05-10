/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_RESOURCE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversions_info.h"
#include "brave/components/brave_ads/core/internal/resources/resource_parsing_error_or.h"

namespace brave_ads {

class ConversionsResource final : public AdsClientNotifierObserver {
 public:
  ConversionsResource();

  ConversionsResource(const ConversionsResource&) = delete;
  ConversionsResource& operator=(const ConversionsResource&) = delete;

  ConversionsResource(ConversionsResource&&) noexcept = delete;
  ConversionsResource& operator=(ConversionsResource&&) noexcept = delete;

  ~ConversionsResource() override;

  bool IsInitialized() const { return is_initialized_; }

  void Load();

  const ConversionsInfo& get() const { return conversions_; }

 private:
  void LoadAndParseResourceCallback(
      ResourceParsingErrorOr<ConversionsInfo> result);

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyDidUpdateResourceComponent(const std::string& id) override;

  bool is_initialized_ = false;

  ConversionsInfo conversions_;

  base::WeakPtrFactory<ConversionsResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_RESOURCE_H_

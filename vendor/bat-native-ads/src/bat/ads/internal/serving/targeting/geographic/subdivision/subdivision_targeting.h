/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_TARGETING_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_TARGETING_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_

#include <string>

#include "bat/ads/internal/base/backoff_timer.h"
#include "bat/ads/internal/base/timer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

namespace targeting {
namespace geographic {

class SubdivisionTargeting final {
 public:
  SubdivisionTargeting();
  ~SubdivisionTargeting();

  void OnPrefChanged(const std::string& path);

  bool ShouldAllowForLocale(const std::string& locale) const;

  bool IsDisabled() const;

  void MaybeFetchForLocale(const std::string& locale);
  void MaybeFetchForCurrentLocale();

  std::string GetSubdivisionCode() const;

 private:
  void OnAutoDetectedAdsSubdivisionTargetingCodePrefChanged();
  void OnAdsSubdivisionTargetingCodePrefChanged();

  std::string GetLazyAutoDetectedSubdivisionCode() const;
  std::string GetLazySubdivisionCode() const;

  bool IsSupportedLocale(const std::string& locale) const;
  void MaybeAllowForLocale(const std::string& locale);

  bool ShouldAutoDetect() const;

  void Fetch();
  void OnFetch(const mojom::UrlResponse& url_response);
  bool ParseJson(const std::string& json);
  void Retry();
  void FetchAfterDelay();

  Timer timer_;
  BackoffTimer retry_timer_;

  mutable absl::optional<std::string> auto_detected_subdivision_code_optional_;
  mutable absl::optional<std::string> subdivision_code_optional_;
};

}  // namespace geographic
}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_TARGETING_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_

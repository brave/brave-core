/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_

#include <string>

#include "absl/types/optional.h"
#include "bat/ads/internal/common/timer/backoff_timer.h"
#include "bat/ads/internal/common/timer/timer.h"
#include "bat/ads/internal/locale/locale_manager_observer.h"
#include "bat/ads/internal/prefs/pref_manager_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads::geographic {

class SubdivisionTargeting final : public LocaleManagerObserver,
                                   public PrefManagerObserver {
 public:
  SubdivisionTargeting();

  SubdivisionTargeting(const SubdivisionTargeting& other) = delete;
  SubdivisionTargeting& operator=(const SubdivisionTargeting& other) = delete;

  SubdivisionTargeting(SubdivisionTargeting&& other) noexcept = delete;
  SubdivisionTargeting& operator=(SubdivisionTargeting&& other) noexcept =
      delete;

  ~SubdivisionTargeting() override;

  static bool ShouldAllow();

  bool IsDisabled() const;

  bool ShouldAutoDetect() const;

  void MaybeAllow();

  void MaybeFetch();

  const std::string& GetSubdivisionCode() const;

 private:
  void OnAutoDetectedSubdivisionTargetingCodePrefChanged();
  void OnSubdivisionTargetingCodePrefChanged();

  const std::string& GetLazyAutoDetectedSubdivisionCode() const;
  const std::string& GetLazySubdivisionCode() const;

  void MaybeAllowForLocale(const std::string& locale);
  void MaybeResetSubdivisionCodeToAutoDetect();
  void MaybeResetSubdivisionCodeToDisabled();

  void MaybeFetchForLocale(const std::string& locale);
  void Fetch();
  void OnFetch(const mojom::UrlResponseInfo& url_response);
  bool ParseJson(const std::string& json);
  void Retry();
  void FetchAfterDelay();

  // LocaleManagerObserver:
  void OnLocaleDidChange(const std::string& locale) override;

  // PrefManagerObserver:
  void OnPrefDidChange(const std::string& path) override;

  Timer timer_;
  BackoffTimer retry_timer_;

  mutable absl::optional<std::string> auto_detected_subdivision_code_;
  mutable absl::optional<std::string> subdivision_code_;
};

}  // namespace ads::geographic

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_

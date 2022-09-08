/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_

#include <string>

#include "bat/ads/internal/base/timer/backoff_timer.h"
#include "bat/ads/internal/base/timer/timer.h"
#include "bat/ads/internal/locale/locale_manager_observer.h"
#include "bat/ads/internal/prefs/pref_manager_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace geographic {

class SubdivisionTargeting final : public LocaleManagerObserver,
                                   public PrefManagerObserver {
 public:
  SubdivisionTargeting();
  SubdivisionTargeting(const SubdivisionTargeting&) = delete;
  SubdivisionTargeting& operator=(const SubdivisionTargeting&) = delete;
  ~SubdivisionTargeting() override;

  bool ShouldAllow() const;

  bool IsDisabled() const;

  void MaybeAllow();

  void MaybeFetch();

  const std::string& GetSubdivisionCode() const;

 private:
  void OnAutoDetectedSubdivisionTargetingCodePrefChanged();
  void OnSubdivisionTargetingCodePrefChanged();

  const std::string& GetLazyAutoDetectedSubdivisionCode() const;
  const std::string& GetLazySubdivisionCode() const;

  bool IsSupportedLocale(const std::string& locale) const;
  void MaybeAllowForLocale(const std::string& locale);
  void MaybeResetSubdivisionCodeToAutoDetect();

  bool ShouldAutoDetect() const;

  void MaybeFetchForLocale(const std::string& locale);
  void Fetch();
  void OnFetch(const mojom::UrlResponseInfo& url_response);
  bool ParseJson(const std::string& json);
  void Retry();
  void FetchAfterDelay();

  // LocaleManagerObserver:
  void OnLocaleDidChange(const std::string& locale) override;

  // PrefManagerObserver:
  void OnPrefChanged(const std::string& path) override;

  Timer timer_;
  BackoffTimer retry_timer_;

  mutable absl::optional<std::string> auto_detected_subdivision_code_;
  mutable absl::optional<std::string> subdivision_code_;
};

}  // namespace geographic
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_

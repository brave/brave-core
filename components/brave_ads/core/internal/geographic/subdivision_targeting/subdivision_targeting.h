/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GEOGRAPHIC_SUBDIVISION_TARGETING_SUBDIVISION_TARGETING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GEOGRAPHIC_SUBDIVISION_TARGETING_SUBDIVISION_TARGETING_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

class SubdivisionTargeting final : public AdsClientNotifierObserver {
 public:
  SubdivisionTargeting();

  SubdivisionTargeting(const SubdivisionTargeting&) = delete;
  SubdivisionTargeting& operator=(const SubdivisionTargeting&) = delete;

  SubdivisionTargeting(SubdivisionTargeting&&) noexcept = delete;
  SubdivisionTargeting& operator=(SubdivisionTargeting&&) noexcept = delete;

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
  void FetchCallback(const mojom::UrlResponseInfo& url_response);
  bool ParseJson(const std::string& json);
  void Retry();
  void FetchAfterDelay();

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyPrefDidChange(const std::string& path) override;

  Timer timer_;
  BackoffTimer retry_timer_;

  mutable absl::optional<std::string> auto_detected_subdivision_code_;
  mutable absl::optional<std::string> subdivision_code_;

  base::WeakPtrFactory<SubdivisionTargeting> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GEOGRAPHIC_SUBDIVISION_TARGETING_SUBDIVISION_TARGETING_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUBDIVISION_TARGETING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUBDIVISION_TARGETING_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_url_request_delegate.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

class SubdivisionUrlRequest;

class SubdivisionTargeting final : public AdsClientNotifierObserver,
                                   public SubdivisionUrlRequestDelegate {
 public:
  SubdivisionTargeting();

  SubdivisionTargeting(const SubdivisionTargeting&) = delete;
  SubdivisionTargeting& operator=(const SubdivisionTargeting&) = delete;

  SubdivisionTargeting(SubdivisionTargeting&&) noexcept = delete;
  SubdivisionTargeting& operator=(SubdivisionTargeting&&) noexcept = delete;

  ~SubdivisionTargeting() override;

  bool IsDisabled() const;

  bool ShouldAutoDetect() const;

  static bool ShouldAllow();

  const std::string& GetSubdivision() const;

 private:
  void Initialize();

  void MaybeRequireSubdivision();
  void InitializeSubdivisionUrlRequest();
  void ShutdownSubdivisionUrlRequest();

  void DisableSubdivision();

  void AutoDetectSubdivision();

  void MaybeAllowForLocale(const std::string& locale);

  bool ShouldFetchSubdivision();
  void MaybeFetchSubdivision();

  void MaybeAllowAndFetchSubdivisionForLocale(const std::string& locale);

  void SetAutoDetectedSubdivision(const std::string& subdivision);
  void UpdateAutoDetectedSubdivision();
  const std::string& GetLazyAutoDetectedSubdivision() const;

  void SetSubdivision(const std::string& subdivision);
  void UpdateSubdivision();
  const std::string& GetLazySubdivision() const;

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyPrefDidChange(const std::string& path) override;

  // SubdivisionUrlRequestDelegate:
  void OnDidFetchSubdivision(const std::string& subdivision) override;

  std::unique_ptr<SubdivisionUrlRequest> subdivision_url_request_;

  mutable absl::optional<std::string> auto_detected_subdivision_;
  mutable absl::optional<std::string> subdivision_;

  base::WeakPtrFactory<SubdivisionTargeting> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUBDIVISION_TARGETING_H_

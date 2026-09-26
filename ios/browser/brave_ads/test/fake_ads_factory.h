/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_TEST_FAKE_ADS_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_TEST_FAKE_ADS_FACTORY_H_

#include <cstddef>
#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/ios/browser/brave_ads/ads_factory.h"

namespace brave_ads {
class AdsMock;
}  // namespace brave_ads

namespace brave_ads::test {

// Fake implementation of `AdsFactory` that records how many `Ads` engines have
// been created and controls whether subsequently created engines report
// initialization/shutdown success.
class FakeAdsFactory final : public AdsFactory {
 public:
  FakeAdsFactory();

  FakeAdsFactory(const FakeAdsFactory&) = delete;
  FakeAdsFactory& operator=(const FakeAdsFactory&) = delete;

  ~FakeAdsFactory() override;

  AdsMock* GetAds() { return ads_.get(); }

  size_t create_count() const { return create_count_; }

  // Causes subsequently created `AdsMock` engine to report initialization
  // failure.
  void set_simulate_initialization_failure() {
    simulate_initialization_failure_ = true;
  }

  // Causes subsequently created `AdsMock` engine to report shutdown failure.
  // Must be called before the `InitializeAds` call whose engine should fail
  // to shut down.
  void set_simulate_shutdown_failure() { simulate_shutdown_failure_ = true; }

  // AdsFactory:
  std::unique_ptr<Ads> CreateAds(
      AdsClient& ads_client,
      const base::FilePath& database_path) override;

 private:
  base::WeakPtr<AdsMock> ads_;
  size_t create_count_ = 0;

  bool simulate_initialization_failure_ = false;
  bool simulate_shutdown_failure_ = false;
};

}  // namespace brave_ads::test

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_TEST_FAKE_ADS_FACTORY_H_

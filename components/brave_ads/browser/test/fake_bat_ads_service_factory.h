/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_SERVICE_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_SERVICE_FACTORY_H_

#include <cstddef>
#include <memory>

#include "brave/components/brave_ads/browser/bat_ads_service_factory.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_ads::test {

class FakeBatAdsService;

// Records launch, initialization, and shutdown counts and controls whether
// initialization succeeds, letting tests observe the full bat ads service
// lifecycle.
class FakeBatAdsServiceFactory : public BatAdsServiceFactory {
 public:
  FakeBatAdsServiceFactory();

  FakeBatAdsServiceFactory(const FakeBatAdsServiceFactory&) = delete;
  FakeBatAdsServiceFactory& operator=(const FakeBatAdsServiceFactory&) = delete;

  ~FakeBatAdsServiceFactory() override;

  size_t launch_count() const { return launch_count_; }
  size_t initialize_count() const { return initialize_count_; }
  size_t shutdown_count() const { return shutdown_count_; }

  // Causes subsequently launched services to report initialization failure.
  void set_simulate_initialization_failure() {
    simulate_initialization_failure_ = true;
  }

  // BatAdsServiceFactory:
  mojo::Remote<bat_ads::mojom::BatAdsService> Launch() const override;

 private:
  void OnInitialize() const { ++initialize_count_; }
  void OnShutdown() const { ++shutdown_count_; }

  // `mutable` because `Launch`, `OnInitialize`, and `OnShutdown` are `const`
  // per the base class interface but must increment these counters.
  mutable size_t launch_count_ = 0;
  mutable size_t initialize_count_ = 0;
  mutable size_t shutdown_count_ = 0;

  bool simulate_initialization_failure_ = false;

  // Owns the service implementation so the receiver stays alive for as long
  // as the `mojo::Remote` returned by `Launch` is in use.
  mutable std::unique_ptr<FakeBatAdsService> bat_ads_service_;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_SERVICE_FACTORY_H_

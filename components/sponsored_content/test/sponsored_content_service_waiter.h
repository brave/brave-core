/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_TEST_SPONSORED_CONTENT_SERVICE_WAITER_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_TEST_SPONSORED_CONTENT_SERVICE_WAITER_H_

#include "base/memory/raw_ref.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/types/optional_ref.h"
#include "brave/components/sponsored_content/sponsored_content_service_observer.h"

namespace sponsored_content {

class SponsoredContentService;

}  // namespace sponsored_content

namespace sponsored_content::test {

// Observes `SponsoredContentService` and waits for updates in tests.
class SponsoredContentServiceWaiter final
    : public SponsoredContentServiceObserver {
 public:
  explicit SponsoredContentServiceWaiter(SponsoredContentService& service);

  SponsoredContentServiceWaiter(const SponsoredContentServiceWaiter&) = delete;
  SponsoredContentServiceWaiter& operator=(
      const SponsoredContentServiceWaiter&) = delete;

  ~SponsoredContentServiceWaiter() override;

  // Blocks until the service notifies a `campaigns.json` update.
  void WaitForOnSponsoredNewTabTakeoverContentDidUpdate();

  // Blocks until the service notifies a `tiles.json` update.
  void WaitForOnSponsoredSiteContentDidUpdate();

  // Blocks until the service notifies a `conversion-config.json` update.
  void WaitForOnSponsoredConversionConfigContentDidUpdate();

 private:
  // SponsoredContentServiceObserver:
  void OnSponsoredNewTabTakeoverContentDidUpdate(
      const base::FilePath& installed_dir,
      base::optional_ref<const base::DictValue> dict) override;
  void OnSponsoredSiteContentDidUpdate(
      const base::FilePath& installed_dir,
      base::optional_ref<const base::DictValue> dict) override;
  void OnSponsoredConversionConfigContentDidUpdate(
      const base::FilePath& installed_dir,
      base::optional_ref<const base::DictValue> dict) override;

  const raw_ref<SponsoredContentService> service_;

  base::RunLoop on_sponsored_new_tab_takeover_content_did_update_run_loop_;
  base::RunLoop on_sponsored_site_content_did_update_run_loop_;
  base::RunLoop on_sponsored_conversion_config_content_did_update_run_loop_;

  base::ScopedObservation<SponsoredContentService,
                          SponsoredContentServiceObserver>
      observation_{this};
};

}  // namespace sponsored_content::test

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_TEST_SPONSORED_CONTENT_SERVICE_WAITER_H_

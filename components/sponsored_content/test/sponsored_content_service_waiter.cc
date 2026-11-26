/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/test/sponsored_content_service_waiter.h"

#include "brave/components/sponsored_content/sponsored_content_service.h"

namespace sponsored_content::test {

SponsoredContentServiceWaiter::SponsoredContentServiceWaiter(
    SponsoredContentService& service)
    : service_(service) {
  observation_.Observe(&service);
}

SponsoredContentServiceWaiter::~SponsoredContentServiceWaiter() = default;

void SponsoredContentServiceWaiter::
    WaitForOnSponsoredNewTabTakeoverContentDidUpdate() {
  on_sponsored_new_tab_takeover_content_did_update_run_loop_.Run();
}

void SponsoredContentServiceWaiter::WaitForOnSponsoredSiteContentDidUpdate() {
  on_sponsored_site_content_did_update_run_loop_.Run();
}

void SponsoredContentServiceWaiter::
    WaitForOnSponsoredConversionConfigContentDidUpdate() {
  on_sponsored_conversion_config_content_did_update_run_loop_.Run();
}

///////////////////////////////////////////////////////////////////////////////

void SponsoredContentServiceWaiter::OnSponsoredNewTabTakeoverContentDidUpdate(
    const base::FilePath& /*installed_dir*/,
    base::optional_ref<const base::DictValue> /*dict*/) {
  on_sponsored_new_tab_takeover_content_did_update_run_loop_.Quit();
}

void SponsoredContentServiceWaiter::OnSponsoredSiteContentDidUpdate(
    const base::FilePath& /*installed_dir*/,
    base::optional_ref<const base::DictValue> /*dict*/) {
  on_sponsored_site_content_did_update_run_loop_.Quit();
}

void SponsoredContentServiceWaiter::OnSponsoredConversionConfigContentDidUpdate(
    const base::FilePath& /*installed_dir*/,
    base::optional_ref<const base::DictValue> /*dict*/) {
  on_sponsored_conversion_config_content_did_update_run_loop_.Quit();
}

}  // namespace sponsored_content::test

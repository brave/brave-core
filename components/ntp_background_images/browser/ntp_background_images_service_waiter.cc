/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_service_waiter.h"

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

namespace ntp_background_images {

NTPBackgroundImagesServiceWaiter::NTPBackgroundImagesServiceWaiter(
    NTPBackgroundImagesService& service)
    : service_(service) {
  service_->AddObserver(this);
}

NTPBackgroundImagesServiceWaiter::~NTPBackgroundImagesServiceWaiter() {
  service_->RemoveObserver(this);
}

void NTPBackgroundImagesServiceWaiter::WaitForOnSponsoredImagesUpdated() {
  on_sponsored_images_updated_run_loop_.Run();
}

void NTPBackgroundImagesServiceWaiter::WaitForOnBackgroundImagesUpdated() {
  on_background_images_updated_run_loop_.Run();
}

void NTPBackgroundImagesServiceWaiter::WaitForOnSponsoredContentDidUpdate() {
  on_sponsored_content_did_update_run_loop_.Run();
}

void NTPBackgroundImagesServiceWaiter::WaitForOnSuperReferralEnded() {
  on_super_referral_ended_run_loop_.Run();
}

///////////////////////////////////////////////////////////////////////////////

void NTPBackgroundImagesServiceWaiter::OnUpdated(NTPSponsoredImagesData* data) {
  on_sponsored_images_updated_run_loop_.Quit();
}

void NTPBackgroundImagesServiceWaiter::OnUpdated(
    NTPBackgroundImagesData* data) {
  on_background_images_updated_run_loop_.Quit();
}

void NTPBackgroundImagesServiceWaiter::OnSponsoredContentDidUpdate(
    const base::Value::Dict& data) {
  on_sponsored_content_did_update_run_loop_.Quit();
}

void NTPBackgroundImagesServiceWaiter::OnSuperReferralEnded() {
  on_super_referral_ended_run_loop_.Quit();
}

}  // namespace ntp_background_images

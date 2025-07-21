/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_WAITER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_WAITER_H_

#include "base/memory/raw_ref.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

namespace ntp_background_images {

struct NTPBackgroundImagesData;
struct NTPSponsoredImagesData;

class NTPBackgroundImagesServiceWaiter
    : public NTPBackgroundImagesService::Observer {
 public:
  explicit NTPBackgroundImagesServiceWaiter(
      NTPBackgroundImagesService& service);

  NTPBackgroundImagesServiceWaiter(const NTPBackgroundImagesServiceWaiter&) =
      delete;
  NTPBackgroundImagesServiceWaiter& operator=(
      const NTPBackgroundImagesServiceWaiter&) = delete;

  ~NTPBackgroundImagesServiceWaiter() override;

  void WaitForOnBackgroundImagesDataDidUpdate();
  void WaitForOnSponsoredImagesDataDidUpdate();
  void WaitForOnSponsoredContentDidUpdate();

 private:
  // NTPBackgroundImagesService::Observer:
  void OnBackgroundImagesDataDidUpdate(NTPBackgroundImagesData* data) override;
  void OnSponsoredImagesDataDidUpdate(NTPSponsoredImagesData* data) override;
  void OnSponsoredContentDidUpdate(const base::Value::Dict& dict) override;

  const raw_ref<NTPBackgroundImagesService> service_;

  base::RunLoop on_background_images_did_update_run_loop_;
  base::RunLoop on_sponsored_images_data_did_update_run_loop_;
  base::RunLoop on_sponsored_content_did_update_run_loop_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_SERVICE_WAITER_H_

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/new_tab_takeover_ui/new_tab_takeover_ui_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "brave/components/new_tab_takeover/mojom/new_tab_takeover.mojom.h"
#include "brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

NewTabTakeoverUIHandler::NewTabTakeoverUIHandler(
    ntp_background_images::NTPBackgroundImagesService*
        ntp_background_images_service)
    : ntp_background_images_service_(ntp_background_images_service) {}

NewTabTakeoverUIHandler::~NewTabTakeoverUIHandler() = default;

void NewTabTakeoverUIHandler::BindInterface(
    mojo::PendingReceiver<new_tab_takeover::mojom::NewTabTakeover>
        pending_receiver) {
  if (receiver_.is_bound()) {
    receiver_.reset();
  }

  receiver_.Bind(std::move(pending_receiver));
}

///////////////////////////////////////////////////////////////////////////////

void NewTabTakeoverUIHandler::SetSponsoredRichMediaAdEventHandler(
    mojo::PendingReceiver<
        ntp_background_images::mojom::SponsoredRichMediaAdEventHandler>
        event_handler) {
  // rich_media_ad_event_handler_->Bind(std::move(event_handler));
}

void NewTabTakeoverUIHandler::GetCurrentWallpaper(
    const std::string& creative_instance_id,
    GetCurrentWallpaperCallback callback) {
  auto failed = [&callback]() {
    std::move(callback).Run(
        /*url=*/std::nullopt,
        brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
        /*target_url=*/std::nullopt);
  };

  const ntp_background_images::NTPSponsoredImagesData* sponsored_images_data =
      ntp_background_images_service_->GetSponsoredImagesData(
          /*supports_rich_media=*/true);
  if (!sponsored_images_data) {
    return failed();
  }

  const ntp_background_images::Creative* creative =
      sponsored_images_data->GetCreativeByInstanceId(creative_instance_id);
  if (!creative) {
    return failed();
  }

  std::move(callback).Run(creative->url, creative->metric_type,
                          GURL(creative->logo.destination_url));
}

void NewTabTakeoverUIHandler::NavigateToUrl(const GURL& url) {
  // TODO(aseren): Implement Navigation.
}

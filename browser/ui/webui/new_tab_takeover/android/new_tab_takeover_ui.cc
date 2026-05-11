// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui.h"

#include <memory>
#include <ranges>
#include <string>
#include <utility>

#include "base/logging.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/new_tab_takeover/grit/new_tab_takeover_generated_map.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace {

content::WebContents* GetActiveWebContents() {
  const TabModelList::TabModelVector& tab_models = TabModelList::models();
  const auto iter = std::ranges::find_if(
      tab_models, [](const auto& model) { return model->IsActiveModel(); });
  if (iter == tab_models.cend()) {
    return nullptr;
  }

  return (*iter)->GetActiveWebContents();
}

}  // namespace

NewTabTakeoverUI::NewTabTakeoverUI(
    content::WebUI* const web_ui,
    ntp_background_images::NTPBackgroundImagesService&
        ntp_background_images_service,
    std::unique_ptr<ntp_background_images::NTPSponsoredRichMediaAdEventHandler>
        rich_media_ad_event_handler)
    : ui::MojoWebUIController(web_ui),
      ntp_background_images_service_(ntp_background_images_service),
      rich_media_ad_event_handler_(std::move(rich_media_ad_event_handler)) {
  LOG(ERROR) << "SponsoredRichMedia: constructing WebUI controller.";

  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, kNewTabTakeoverHost, kNewTabTakeoverGenerated,
      IDR_NEW_TAB_TAKEOVER_HTML);

  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      absl::StrFormat("frame-src %s;", kNTPNewTabTakeoverRichMediaUrl));
  source->AddString("ntpNewTabTakeoverRichMediaUrl",
                    kNTPNewTabTakeoverRichMediaUrl);

  LOG(ERROR) << "SponsoredRichMedia: data source created, rich_media_url="
             << kNTPNewTabTakeoverRichMediaUrl << ".";
}

NewTabTakeoverUI::~NewTabTakeoverUI() = default;

void NewTabTakeoverUI::BindInterface(
    mojo::PendingReceiver<new_tab_takeover::mojom::NewTabTakeover>
        pending_receiver) {
  LOG(ERROR) << "SponsoredRichMedia: BindInterface called, already_bound="
             << new_tab_takeover_receiver_.is_bound() << ".";

  if (new_tab_takeover_receiver_.is_bound()) {
    new_tab_takeover_receiver_.reset();
  }

  new_tab_takeover_receiver_.Bind(std::move(pending_receiver));

  LOG(ERROR) << "SponsoredRichMedia: Mojo interface bound.";
}

///////////////////////////////////////////////////////////////////////////////

void NewTabTakeoverUI::SetSponsoredRichMediaAdEventHandler(
    mojo::PendingReceiver<
        ntp_background_images::mojom::SponsoredRichMediaAdEventHandler>
        event_handler) {
  LOG(ERROR)
      << "SponsoredRichMedia: SetSponsoredRichMediaAdEventHandler called.";
  rich_media_ad_event_handler_->Bind(std::move(event_handler));
  LOG(ERROR) << "SponsoredRichMedia: ad event handler bound.";
}

void NewTabTakeoverUI::GetCurrentWallpaper(
    const std::string& creative_instance_id,
    GetCurrentWallpaperCallback callback) {
  LOG(ERROR) << "SponsoredRichMedia: GetCurrentWallpaper called, "
                "creative_instance_id="
             << creative_instance_id << ".";

  auto failed = [&callback]() {
    LOG(ERROR) << "SponsoredRichMedia: GetCurrentWallpaper failed, no creative "
                  "data available.";
    std::move(callback).Run(
        /*url=*/std::nullopt,
        brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
        /*target_url=*/std::nullopt);
  };

  const ntp_background_images::NTPSponsoredImagesData* sponsored_images_data =
      ntp_background_images_service_->GetSponsoredImagesData(
          /*supports_rich_media=*/true);
  if (!sponsored_images_data) {
    LOG(ERROR) << "SponsoredRichMedia: GetCurrentWallpaper failed, "
                  "sponsored_images_data is null.";
    return failed();
  }

  const ntp_background_images::Creative* creative =
      sponsored_images_data->GetCreativeByInstanceId(creative_instance_id);
  if (!creative) {
    LOG(ERROR) << "SponsoredRichMedia: GetCurrentWallpaper failed, no creative "
                  "for creative_instance_id="
               << creative_instance_id << ".";
    return failed();
  }

  LOG(ERROR) << "SponsoredRichMedia: GetCurrentWallpaper succeeded, url="
             << creative->url << ".";
  std::move(callback).Run(creative->url, creative->metric_type,
                          GURL(creative->logo.destination_url));
}

void NewTabTakeoverUI::NavigateToUrl(const GURL& url) {
  LOG(ERROR) << "SponsoredRichMedia: NavigateToUrl called, url=" << url << ".";

  // The current New Tab Takeover web contents is displayed in the ThinWebView
  // so it is not connected to the Android Tab, i.e. WebContents::GetDelegate()
  // returns nullptr. Therefore to do a Tab navigation, we need to locate
  // the current Android Tab and open the URL in it.
  content::WebContents* web_contents = GetActiveWebContents();
  if (!web_contents) {
    LOG(ERROR) << "SponsoredRichMedia: NavigateToUrl failed, no active web "
                  "contents found.";
    return;
  }

  LOG(ERROR) << "SponsoredRichMedia: navigating active tab to url=" << url
             << ".";
  const content::OpenURLParams params(
      url, content::Referrer(), WindowOpenDisposition::CURRENT_TAB,
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, /*is_renderer_initiated=*/false);
  web_contents->OpenURL(params, /*navigation_handle_callback=*/{});
}

WEB_UI_CONTROLLER_TYPE_IMPL(NewTabTakeoverUI)

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui.h"

#include <memory>
#include <ranges>
#include <string>
#include <utility>

#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_new_tab_ui/new_tab_takeover/grit/new_tab_takeover_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"

namespace {

content::WebContents* GetActiveWebContents() {
  const TabModelList::TabModelVector& tab_models = TabModelList::models();
  const auto iter = std::ranges::find_if(
      tab_models, [](const auto& model) { return model->IsActiveModel(); });
  if (iter == tab_models.cend()) {
    return nullptr;
  }

  content::WebContents* active_web_contents = (*iter)->GetActiveWebContents();
  if (!active_web_contents) {
    return nullptr;
  }

  return active_web_contents;
}

}  // namespace

NewTabTakeoverUI::NewTabTakeoverUI(
    content::WebUI* const web_ui,
    const std::string& name,
    brave_ads::AdsService* ads_service,
    PrefService* local_state,
    p3a::P3AService* p3a_service,
    ntp_background_images::NTPBackgroundImagesService*
        ntp_background_images_service,
    PrefService* profile_prefs)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, name, kNewTabTakeoverGenerated, IDR_NEW_TAB_TAKEOVER_HTML);

  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StringPrintf("frame-src %s;", kNTPNewTabTakeoverRichMediaUrl));
  source->AddString("ntpNewTabTakeoverRichMediaUrl",
                    kNTPNewTabTakeoverRichMediaUrl);

  auto ntp_p3a_helper =
      std::make_unique<ntp_background_images::NTPP3AHelperImpl>(
          local_state, p3a_service, ntp_background_images_service,
          profile_prefs);

  rich_media_ad_event_handler_ = std::make_unique<
      ntp_background_images::NTPSponsoredRichMediaAdEventHandler>(
      ads_service, std::move(ntp_p3a_helper));
}

NewTabTakeoverUI::~NewTabTakeoverUI() = default;

void NewTabTakeoverUI::BindInterface(
    mojo::PendingReceiver<new_tab_takeover::mojom::NewTabTakeover>
        pending_receiver) {
  if (new_tab_takeover_receiver_.is_bound()) {
    new_tab_takeover_receiver_.reset();
  }

  new_tab_takeover_receiver_.Bind(std::move(pending_receiver));
}

///////////////////////////////////////////////////////////////////////////////

void NewTabTakeoverUI::SetSponsoredRichMediaAdEventHandler(
    mojo::PendingReceiver<
        ntp_background_images::mojom::SponsoredRichMediaAdEventHandler>
        event_handler) {
  rich_media_ad_event_handler_->Bind(std::move(event_handler));
}

void NewTabTakeoverUI::NavigateToUrl(const GURL& url) {
  content::WebContents* web_contents = GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  const content::OpenURLParams params(
      url, content::Referrer(), WindowOpenDisposition::CURRENT_TAB,
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, /*is_renderer_initiated=*/false);
  web_contents->OpenURL(params, {});
}

WEB_UI_CONTROLLER_TYPE_IMPL(NewTabTakeoverUI)

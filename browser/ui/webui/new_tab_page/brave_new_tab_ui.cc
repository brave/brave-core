// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/ntp_background_images/ntp_custom_background_images_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_page_handler.h"
#include "brave/browser/ui/webui/new_tab_page/top_sites_message_handler.h"
#include "brave/components/brave_new_tab/resources/grit/brave_new_tab_generated_map.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_images_source.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_ui_data_source.h"

using ntp_background_images::NTPCustomBackgroundImagesServiceFactory;
using ntp_background_images::NTPCustomImagesSource;

BraveNewTabUI::BraveNewTabUI(content::WebUI* web_ui, const std::string& name)
    : ui::MojoWebUIController(
          web_ui,
          true /* Needed for legacy non-mojom message handler */),
      page_factory_receiver_(this) {
  Profile* profile = Profile::FromWebUI(web_ui);
  web_ui->OverrideTitle(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_NEW_TAB_TITLE));

  if (brave::ShouldNewTabShowBlankpage(profile)) {
    content::WebUIDataSource* source =
        content::WebUIDataSource::Create(name);
    source->SetDefaultResource(IDR_BRAVE_BLANK_NEW_TAB_HTML);
    content::WebUIDataSource::Add(profile, source);
    return;
  }

  // Non blank NTP.
  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, name, kBraveNewTabGenerated, kBraveNewTabGeneratedSize,
      IDR_BRAVE_NEW_TAB_HTML);
  source->AddBoolean("featureCustomBackgroundEnabled",
                     !profile->GetPrefs()->IsManagedPreference(
                         prefs::kNtpCustomBackgroundDict));

  // Let frontend know about feature flags
  source->AddBoolean(
      "featureFlagBraveNewsEnabled",
      base::FeatureList::IsEnabled(brave_today::features::kBraveNewsFeature));
  source->AddBoolean("featureFlagBraveNewsPromptEnabled",
                     base::FeatureList::IsEnabled(
                         brave_today::features::kBraveNewsCardPeekFeature));
  web_ui->AddMessageHandler(
      base::WrapUnique(BraveNewTabMessageHandler::Create(source, profile)));
  web_ui->AddMessageHandler(
      base::WrapUnique(new TopSitesMessageHandler(profile)));

  // For custom background images.
  if (auto* ntp_custom_background_images_service =
          NTPCustomBackgroundImagesServiceFactory::GetForContext(profile)) {
    content::URLDataSource::Add(profile,
                                std::make_unique<NTPCustomImagesSource>(
                                    ntp_custom_background_images_service));
  }
}

BraveNewTabUI::~BraveNewTabUI() = default;

void BraveNewTabUI::BindInterface(
    mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  DCHECK(profile);
  if (base::FeatureList::IsEnabled(brave_today::features::kBraveNewsFeature)) {
    // Wire up JS mojom to service
    auto* brave_news_controller =
        brave_news::BraveNewsControllerFactory::GetForContext(profile);
    if (brave_news_controller) {
      brave_news_controller->Bind(std::move(receiver));
    }
  }
}

void BraveNewTabUI::BindInterface(
    mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandlerFactory>
        pending_receiver) {
  if (page_factory_receiver_.is_bound()) {
    page_factory_receiver_.reset();
  }

  page_factory_receiver_.Bind(std::move(pending_receiver));
}

void BraveNewTabUI::CreatePageHandler(
    mojo::PendingRemote<brave_new_tab_page::mojom::Page> pending_page,
    mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandler>
        pending_page_handler) {
  DCHECK(pending_page.is_valid());
  Profile* profile = Profile::FromWebUI(web_ui());
  page_handler_ = std::make_unique<BraveNewTabPageHandler>(
      std::move(pending_page_handler), std::move(pending_page), profile,
      web_ui()->GetWebContents());
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewTabUI)

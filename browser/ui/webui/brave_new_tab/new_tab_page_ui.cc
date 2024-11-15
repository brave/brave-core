// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab/new_tab_page_ui.h"

#include <utility>

#include "brave/browser/ntp_background/brave_ntp_custom_background_service_factory.h"
#include "brave/browser/ntp_background/custom_background_file_manager.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/ui/webui/brave_new_tab/background_adapter.h"
#include "brave/browser/ui/webui/brave_new_tab/custom_image_chooser.h"
#include "brave/browser/ui/webui/brave_new_tab/new_tab_page_handler.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_new_tab/new_tab_prefs.h"
#include "brave/components/brave_new_tab/resources/grit/brave_new_tab_generated_map.h"
#include "brave/components/brave_private_cdn/private_cdn_request_helper.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_images_source.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/searchbox/realbox_handler.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "ui/base/webui/web_ui_util.h"

namespace brave_new_tab {

namespace {

static constexpr webui::LocalizedString kStrings[] = {
    {"backgroundSettingsTitle", IDS_NEW_TAB_BACKGROUND_SETTINGS_TITLE},
    {"braveBackgroundLabel", IDS_NEW_TAB_BRAVE_BACKGROUND_LABEL},
    {"customBackgroundLabel", IDS_NEW_TAB_CUSTOM_BACKGROUND_LABEL},
    {"customBackgroundTitle", IDS_NEW_TAB_CUSTOM_BACKGROUND_LABEL},
    {"customizeSearchEnginesLink", IDS_NEW_TAB_CUSTOMIZE_SEARCH_ENGINES_LINK},
    {"enabledSearchEnginesLabel", IDS_NEW_TAB_ENABLED_SEARCH_ENGINES_LABEL},
    {"gradientBackgroundLabel", IDS_NEW_TAB_GRADIENT_BACKGROUND_LABEL},
    {"gradientBackgroundTitle", IDS_NEW_TAB_GRADIENT_BACKGROUND_LABEL},
    {"photoCreditsText", IDS_NEW_TAB_PHOTO_CREDITS_TEXT},
    {"randomizeBackgroundLabel", IDS_NEW_TAB_RANDOMIZE_BACKGROUND_LABEL},
    {"searchAskLeoDescription", IDS_OMNIBOX_ASK_LEO_DESCRIPTION},
    {"searchBoxPlaceholderText", IDS_NEW_TAB_SEARCH_BOX_PLACEHOLDER_TEXT},
    {"searchBoxPlaceholderTextBrave",
     IDS_NEW_TAB_SEARCH_BOX_PLACEHOLDER_TEXT_BRAVE},
    {"searchCustomizeEngineListText",
     IDS_NEW_TAB_SEARCH_CUSTOMIZE_ENGINE_LIST_TEXT},
    {"searchSettingsTitle", IDS_NEW_TAB_SEARCH_SETTINGS_TITLE},
    {"searchSuggestionsDismissButtonLabel",
     IDS_NEW_TAB_SEARCH_SUGGESTIONS_DISMISS_BUTTON_LABEL},
    {"searchSuggestionsEnableButtonLabel",
     IDS_NEW_TAB_SEARCH_SUGGESTIONS_ENABLE_BUTTON_LABEL},
    {"searchSuggestionsPromptText", IDS_NEW_TAB_SEARCH_SUGGESTIONS_PROMPT_TEXT},
    {"searchSuggestionsPromptTitle",
     IDS_NEW_TAB_SEARCH_SUGGESTIONS_PROMPT_TITLE},
    {"settingsTitle", IDS_NEW_TAB_SETTINGS_TITLE},
    {"showBackgroundsLabel", IDS_NEW_TAB_SHOW_BACKGROUNDS_LABEL},
    {"showSearchBoxLabel", IDS_NEW_TAB_SHOW_SEARCH_BOX_LABEL},
    {"showSponsoredImagesLabel", IDS_NEW_TAB_SHOW_SPONSORED_IMAGES_LABEL},
    {"solidBackgroundLabel", IDS_NEW_TAB_SOLID_BACKGROUND_LABEL},
    {"solidBackgroundTitle", IDS_NEW_TAB_SOLID_BACKGROUND_LABEL},
    {"uploadBackgroundLabel", IDS_NEW_TAB_UPLOAD_BACKGROUND_LABEL},
};

constexpr auto kPcdnImageLoaderTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_new_tab_pcdn_loader",
                                        R"(
      semantics {
        sender: "Brave New Tab WebUI"
        description: "Fetches resource data from the Brave private CDN."
        trigger: "Loading images on the new tab page."
        data: "No data sent, other than URL of the resource."
        destination: BRAVE_OWNED_SERVICE
      }
      policy {
        cookies_allowed: NO
        setting: "None"
      }
    )");

// Adds support for displaying images stored in the custom background image
// folder.
void AddCustomImageDataSource(Profile* profile) {
  auto* custom_background_service =
      BraveNTPCustomBackgroundServiceFactory::GetForContext(profile);
  if (!custom_background_service) {
    return;
  }
  auto source = std::make_unique<ntp_background_images::NTPCustomImagesSource>(
      custom_background_service);
  content::URLDataSource::Add(profile, std::move(source));
}

bool ShouldShowBlankPage(Profile* profile) {
  if (!profile->IsRegularProfile()) {
    return false;
  }
  auto shows_option = prefs::GetNewTabShowsOption(profile->GetPrefs());
  return shows_option == prefs::NewTabShowsOption::kBlankpage;
}

}  // namespace

NewTabPageUI::NewTabPageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  auto* profile = Profile::FromWebUI(web_ui);

  auto* source = content::WebUIDataSource::CreateAndAdd(
      profile, chrome::kChromeUINewTabHost);

  if (ShouldShowBlankPage(profile)) {
    source->SetDefaultResource(IDR_BRAVE_BLANK_NEW_TAB_HTML);
  } else {
    webui::SetupWebUIDataSource(source, kBraveNewTabGenerated,
                                IDR_BRAVE_NEW_TAB_PAGE_HTML);
  }

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src chrome://resources chrome://theme chrome://background-wallpaper "
      "chrome://custom-wallpaper chrome://branded-wallpaper chrome://favicon2 "
      "blob: data: 'self';");

  AddBackgroundColorToSource(source, web_ui->GetWebContents());
  AddCustomImageDataSource(profile);

  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));

  web_ui->OverrideTitle(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_NEW_TAB_TITLE));

  source->AddLocalizedStrings(kStrings);
}

NewTabPageUI::~NewTabPageUI() = default;

void NewTabPageUI::BindInterface(
    mojo::PendingReceiver<mojom::NewTabPageHandler> pending_receiver) {
  auto* web_contents = web_ui()->GetWebContents();
  auto* profile = Profile::FromWebUI(web_ui());

  auto* prefs = profile->GetPrefs();

  auto image_chooser =
      std::make_unique<CustomImageChooser>(*web_ui()->GetWebContents());

  auto background_adapter = std::make_unique<BackgroundAdapter>(
      std::make_unique<CustomBackgroundFileManager>(profile), *prefs,
      ntp_background_images::ViewCounterServiceFactory::GetForProfile(profile));

  auto pcdn_helper =
      std::make_unique<brave_private_cdn::PrivateCDNRequestHelper>(
          kPcdnImageLoaderTrafficAnnotation, profile->GetURLLoaderFactory());

  page_handler_ = std::make_unique<NewTabPageHandler>(
      std::move(pending_receiver), std::move(image_chooser),
      std::move(background_adapter), std::move(pcdn_helper),
      *tabs::TabInterface::GetFromContents(web_contents), *prefs,
      *TemplateURLServiceFactory::GetForProfile(profile));
}

void NewTabPageUI::BindInterface(
    mojo::PendingReceiver<searchbox::mojom::PageHandler> pending_receiver) {
  realbox_handler_ = std::make_unique<RealboxHandler>(
      std::move(pending_receiver), Profile::FromWebUI(web_ui()),
      web_ui()->GetWebContents(), /*metrics_reporter=*/nullptr,
      /*lens_searchbox_client=*/nullptr, /*omnibox_controller=*/nullptr);
}

WEB_UI_CONTROLLER_TYPE_IMPL(NewTabPageUI)

}  // namespace brave_new_tab

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_initializer.h"

#include <memory>
#include <utility>

#include "base/strings/strcat.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/ntp_background/brave_ntp_custom_background_service_factory.h"
#include "brave/browser/resources/brave_new_tab_page_refresh/grit/brave_new_tab_page_refresh_generated_map.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_images_source.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_syncable_service.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/common/webui_url_constants.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

namespace brave_new_tab_page_refresh {

NewTabPageInitializer::NewTabPageInitializer(content::WebUI& web_ui)
    : web_ui_(web_ui) {}

NewTabPageInitializer::~NewTabPageInitializer() = default;

void NewTabPageInitializer::Initialize() {
  source_ = content::WebUIDataSource::CreateAndAdd(GetProfile(),
                                                   chrome::kChromeUINewTabHost);

  if (brave::ShouldNewTabShowBlankpage(GetProfile())) {
    source_->SetDefaultResource(IDR_BRAVE_BLANK_NEW_TAB_HTML);
  } else {
    webui::SetupWebUIDataSource(source_, kBraveNewTabPageRefreshGenerated,
                                IDR_BRAVE_NEW_TAB_PAGE_HTML);
  }

  AddBackgroundColorToSource(source_, web_ui_->GetWebContents());
  AddCSPOverrides();
  AddLoadTimeValues();
  AddStrings();

  AddFaviconDataSource();
  AddCustomImageDataSource();

  web_ui_->AddRequestableScheme(content::kChromeUIUntrustedScheme);
  web_ui_->OverrideTitle(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_NEW_TAB_TITLE));
}

Profile* NewTabPageInitializer::GetProfile() {
  return Profile::FromWebUI(&web_ui_.get());
}

void NewTabPageInitializer::AddCSPOverrides() {
  source_->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src chrome://resources chrome://theme chrome://background-wallpaper "
      "chrome://custom-wallpaper chrome://branded-wallpaper chrome://favicon2 "
      "blob: data: 'self';");

  source_->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StrCat({"frame-src ", kNTPNewTabTakeoverRichMediaUrl, ";"}));
}

void NewTabPageInitializer::AddLoadTimeValues() {
  auto* profile = GetProfile();

  source_->AddBoolean(
      "customBackgroundFeatureEnabled",
      !profile->GetPrefs()->IsManagedPreference(GetThemePrefNameInMigration(
          ThemePrefInMigration::kNtpCustomBackgroundDict)));

  source_->AddString("sponsoredRichMediaBaseUrl",
                     kNTPNewTabTakeoverRichMediaUrl);
}

void NewTabPageInitializer::AddStrings() {
  static constexpr webui::LocalizedString kStrings[] = {
      {"backgroundSettingsTitle", IDS_NEW_TAB_BACKGROUND_SETTINGS_TITLE},
      {"braveBackgroundLabel", IDS_NEW_TAB_BRAVE_BACKGROUND_LABEL},
      {"customBackgroundLabel", IDS_NEW_TAB_CUSTOM_BACKGROUND_LABEL},
      {"customBackgroundTitle", IDS_NEW_TAB_CUSTOM_BACKGROUND_LABEL},
      {"gradientBackgroundLabel", IDS_NEW_TAB_GRADIENT_BACKGROUND_LABEL},
      {"gradientBackgroundTitle", IDS_NEW_TAB_GRADIENT_BACKGROUND_LABEL},
      {"photoCreditsText", IDS_NEW_TAB_PHOTO_CREDITS_TEXT},
      {"randomizeBackgroundLabel", IDS_NEW_TAB_RANDOMIZE_BACKGROUND_LABEL},
      {"settingsTitle", IDS_NEW_TAB_SETTINGS_TITLE},
      {"showBackgroundsLabel", IDS_NEW_TAB_SHOW_BACKGROUNDS_LABEL},
      {"showSponsoredImagesLabel", IDS_NEW_TAB_SHOW_SPONSORED_IMAGES_LABEL},
      {"solidBackgroundLabel", IDS_NEW_TAB_SOLID_BACKGROUND_LABEL},
      {"solidBackgroundTitle", IDS_NEW_TAB_SOLID_BACKGROUND_LABEL},
      {"uploadBackgroundLabel", IDS_NEW_TAB_UPLOAD_BACKGROUND_LABEL}};

  source_->AddLocalizedStrings(kStrings);
}

void NewTabPageInitializer::AddFaviconDataSource() {
  auto* profile = GetProfile();
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
}

void NewTabPageInitializer::AddCustomImageDataSource() {
  auto* profile = GetProfile();
  auto* custom_background_service =
      BraveNTPCustomBackgroundServiceFactory::GetForContext(profile);
  if (!custom_background_service) {
    return;
  }
  auto source = std::make_unique<ntp_background_images::NTPCustomImagesSource>(
      custom_background_service);
  content::URLDataSource::Add(profile, std::move(source));
}

}  // namespace brave_new_tab_page_refresh

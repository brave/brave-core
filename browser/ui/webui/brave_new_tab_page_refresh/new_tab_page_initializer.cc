// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_initializer.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/ntp_background/brave_ntp_custom_background_service_factory.h"
#include "brave/browser/resources/brave_new_tab_page_refresh/grit/brave_new_tab_page_refresh_generated_map.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/browser/ui/webui/brave_sanitized_image_source.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_images_source.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/regional_capabilities/regional_capabilities_service_factory.h"
#include "chrome/browser/themes/theme_syncable_service.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/plural_string_handler.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/ntp_tiles/constants.h"
#include "components/prefs/pref_service.h"
#include "components/regional_capabilities/regional_capabilities_country_id.h"
#include "components/regional_capabilities/regional_capabilities_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/webui/webui_util.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#endif

namespace brave_new_tab_page_refresh {

namespace {

using regional_capabilities::RegionalCapabilitiesServiceFactory;

constexpr char kBraveSearchHost[] = "search.brave.com";
constexpr char kYahooSearchHost[] = "search.yahoo.co.jp";

}  // namespace

std::string_view GetSearchDefaultHost(
    regional_capabilities::RegionalCapabilitiesService* regional_capabilities) {
  CHECK(regional_capabilities);
  regional_capabilities::CountryIdHolder country_id =
      regional_capabilities->GetCountryId();
  regional_capabilities::CountryIdHolder japan_country_id(
      country_codes::CountryId("JP"));
  if (country_id == japan_country_id) {
    return kYahooSearchHost;
  }

  return kBraveSearchHost;
}

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
  AddPluralStrings();
  AddResourcePaths();

  AddFaviconDataSource();
  AddCustomImageDataSource();
  AddSanitizedImageDataSource();
  MaybeMigrateHideAllWidgetsPref();

  web_ui_->AddRequestableScheme(content::kChromeUIUntrustedScheme);
  web_ui_->OverrideTitle(l10n_util::GetStringUTF16(IDS_NEW_TAB_TITLE));

  content::URLDataSource::Add(GetProfile(),
                              std::make_unique<ThemeSource>(GetProfile()));
}

Profile* NewTabPageInitializer::GetProfile() {
  return Profile::FromWebUI(&web_ui_.get());
}

void NewTabPageInitializer::AddCSPOverrides() {
  source_->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src chrome://brave-image chrome://resources chrome://theme "
      "chrome://background-wallpaper chrome://custom-wallpaper "
      "chrome://branded-wallpaper chrome://favicon2 blob: data: 'self';");

  source_->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StrCat({"frame-src ", kNTPNewTabTakeoverRichMediaUrl, ";"}));
}

void NewTabPageInitializer::AddLoadTimeValues() {
  auto* profile = GetProfile();
  auto* prefs = profile->GetPrefs();

  source_->AddBoolean("customBackgroundFeatureEnabled",
                      !profile->GetPrefs()->IsManagedPreference(
                          prefs::kNtpCustomBackgroundDict));

  source_->AddString("sponsoredRichMediaBaseUrl",
                     kNTPNewTabTakeoverRichMediaUrl);

  source_->AddBoolean(
      "ntpSearchFeatureEnabled",
      base::FeatureList::IsEnabled(features::kBraveNtpSearchWidget));

  source_->AddString(
      "ntpSearchDefaultHost",
      GetSearchDefaultHost(
          RegionalCapabilitiesServiceFactory::GetForProfile(profile)));

  source_->AddBoolean("rewardsFeatureEnabled",
                      brave_rewards::IsSupportedForProfile(profile));

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  bool vpn_feature_enabled = brave_vpn::IsBraveVPNEnabled(prefs);
#else
  bool vpn_feature_enabled = false;
#endif
  source_->AddBoolean("vpnFeatureEnabled", vpn_feature_enabled);

  bool news_feed_update_enabled =
      base::FeatureList::IsEnabled(brave_news::features::kBraveNewsFeedUpdate);
  source_->AddBoolean("featureFlagBraveNewsFeedV2Enabled",
                      news_feed_update_enabled);

  source_->AddBoolean(
      "newsFeatureEnabled",
      news_feed_update_enabled &&
          !prefs->GetBoolean(brave_news::prefs::kBraveNewsDisabledByPolicy));

  source_->AddBoolean("talkFeatureEnabled",
                      !prefs->GetBoolean(kBraveTalkDisabledByPolicy));

  source_->AddInteger("maxCustomTopSites", ntp_tiles::kMaxNumCustomLinks);
}

void NewTabPageInitializer::AddStrings() {
  source_->AddLocalizedStrings(webui::kBraveNewTabPageStrings);
  source_->AddLocalizedStrings(webui::kBraveNewsStrings);
  source_->AddLocalizedStrings(webui::kBraveRewardsStrings);
  source_->AddLocalizedStrings(webui::kBraveOmniboxStrings);
}

void NewTabPageInitializer::AddPluralStrings() {
  auto handler = std::make_unique<PluralStringHandler>();
  handler->AddLocalizedString("BRAVE_NEWS_SOURCE_COUNT",
                              IDS_BRAVE_NEWS_SOURCE_COUNT);
  handler->AddLocalizedString("REWARDS_CONNECTED_ADS_VIEWED_TEXT",
                              IDS_REWARDS_CONNECTED_ADS_VIEWED_TEXT);
  web_ui_->AddMessageHandler(std::move(handler));
}

void NewTabPageInitializer::AddResourcePaths() {
  source_->AddResourcePaths(
      {{"dylan-malval_sea-min.webp", IDR_BRAVE_NEW_TAB_BACKGROUND1}});
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

void NewTabPageInitializer::AddSanitizedImageDataSource() {
  auto* profile = GetProfile();
  content::URLDataSource::Add(
      profile, std::make_unique<BraveSanitizedImageSource>(profile));
}

void NewTabPageInitializer::MaybeMigrateHideAllWidgetsPref() {
  // The "hide all widgets" toggle does not exist on this version of the NTP.
  // If the user has enabled this pref, hide the individual widgets affected by
  // that pref.
  // TODO(https://github.com/brave/brave-browser/issues/49544): Deprecate the
  // `kNewTabPageHideAllWidgets` pref and perform the migration in
  // `MigrateObsoleteProfilePrefs`.
  auto* prefs = GetProfile()->GetPrefs();
  if (prefs->GetBoolean(kNewTabPageHideAllWidgets)) {
    prefs->SetBoolean(kNewTabPageHideAllWidgets, false);

    prefs->SetBoolean(kNewTabPageShowRewards, false);
    prefs->SetBoolean(kNewTabPageShowBraveTalk, false);
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    prefs->SetBoolean(kNewTabPageShowBraveVPN, false);
#endif
  }
}

}  // namespace brave_new_tab_page_refresh

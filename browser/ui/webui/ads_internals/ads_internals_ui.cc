// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ads_internals/ads_internals_ui.h"

#include <optional>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/browser/component_updater/component_util.h"
#include "brave/components/brave_ads/browser/resources/grit/ads_internals_generated_map.h"
#include "brave/components/brave_ads/core/public/ads_internals/ads_internals_verbose_mode_feature.h"
#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/new_tab_page/prefs/ntp_pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/component_updater/component_updater_service.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "components/variations/service/variations_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "base/feature_list.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/core/features.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

namespace {

brave_rewards::RewardsService* GetRewardsServiceForWebUI(
    content::WebUI* web_ui) {
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  return brave_rewards::RewardsServiceFactory::GetForProfile(
      Profile::FromWebUI(web_ui));
#else
  return nullptr;
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)
}

bool IsVerboseLoggingEnabled() {
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  return base::FeatureList::IsEnabled(
      brave_rewards::features::kVerboseLoggingFeature);
#else
  return false;
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)
}

AdsInternalsHandler::GetComponentIdCallback
GetNtpSponsoredImagesComponentIdCallback() {
  auto* const ntp_background_images_service =
      g_brave_browser_process->ntp_background_images_service();
  if (!ntp_background_images_service) {
    return AdsInternalsHandler::GetComponentIdCallback();
  }

  return base::BindRepeating(
      [](ntp_background_images::NTPBackgroundImagesService* service) {
        return service->GetSponsoredImagesComponentId();
      },
      base::Unretained(ntp_background_images_service));
}

AdsInternalsHandler::GetComponentIdCallback
GetCountryResourceComponentIdCallback() {
  return base::BindRepeating([]() -> std::optional<std::string> {
    // The text classification, purchase intent, and anti-targeting resources
    // all ship inside the two component-updater components below (one per
    // country, one per language) rather than having their own individual
    // component IDs. See `ResourceComponent::RegisterCountryComponent`/
    // `RegisterLanguageComponent`.
    const std::optional<brave_ads::ComponentInfo> component =
        brave_ads::GetComponent(brave_ads::CurrentCountryCode());
    if (!component) {
      return std::nullopt;
    }
    return std::string(component->id);
  });
}

AdsInternalsHandler::GetComponentIdCallback
GetLanguageResourceComponentIdCallback() {
  return base::BindRepeating([]() -> std::optional<std::string> {
    // Same rationale as `GetCountryResourceComponentIdCallback` above.
    const std::optional<brave_ads::ComponentInfo> component =
        brave_ads::GetComponent(brave_ads::CurrentLanguageCode());
    if (!component) {
      return std::nullopt;
    }
    return std::string(component->id);
  });
}

AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback
GetIsSponsoredImagesLoadedCallback() {
  auto* const ntp_background_images_service =
      g_brave_browser_process->ntp_background_images_service();
  if (!ntp_background_images_service) {
    return AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback();
  }

  return base::BindRepeating(
      [](ntp_background_images::NTPBackgroundImagesService* service) {
        return service->GetSponsoredImagesData(
                   /*supports_rich_media=*/true) != nullptr;
      },
      base::Unretained(ntp_background_images_service));
}

AdsInternalsHandler::GetComponentIdCallback
GetNtpSponsoredImagesManifestVersionCallback() {
  auto* const ntp_background_images_service =
      g_brave_browser_process->ntp_background_images_service();
  auto* const component_update_service = g_browser_process->component_updater();
  if (!ntp_background_images_service || !component_update_service) {
    return AdsInternalsHandler::GetComponentIdCallback();
  }

  return base::BindRepeating(
      [](ntp_background_images::NTPBackgroundImagesService* service,
         component_updater::ComponentUpdateService* component_update_service)
          -> std::optional<std::string> {
        const std::optional<std::string>& component_id =
            service->GetSponsoredImagesComponentId();
        if (!component_id) {
          return std::nullopt;
        }

        for (const auto& component :
             component_update_service->GetComponents()) {
          if (component.id == *component_id) {
            return component.version.GetString();
          }
        }

        return std::nullopt;
      },
      base::Unretained(ntp_background_images_service),
      base::Unretained(component_update_service));
}

AdsInternalsHandler::GetIsSponsoredTilesShownCallback
GetIsSponsoredTilesShownCallback(PrefService* prefs) {
  return base::BindRepeating(
      [](PrefService* prefs) {
        return prefs->GetBoolean(ntp_prefs::kNtpShortcutsVisible) &&
               prefs->GetBoolean(brave_ads::prefs::kSponsoredEnabled);
      },
      base::Unretained(prefs));
}

}  // namespace

bool AdsInternalsUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  auto* profile = Profile::FromBrowserContext(browser_context);
  return !profile->IsIncognitoProfile() &&
         !profile->GetPrefs()->GetBoolean(
             brave_rewards::prefs::kDisabledByPolicy);
}

AdsInternalsUI::AdsInternalsUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui),
      handler_(brave_ads::AdsServiceFactory::GetForProfile(
                   Profile::FromWebUI(web_ui)),
               *Profile::FromWebUI(web_ui)->GetPrefs(),
               g_browser_process->variations_service(),
               GetNtpSponsoredImagesComponentIdCallback(),
               GetCountryResourceComponentIdCallback(),
               GetLanguageResourceComponentIdCallback(),
               GetIsSponsoredImagesLoadedCallback(),
               GetNtpSponsoredImagesManifestVersionCallback(),
               GetIsSponsoredTilesShownCallback(
                   Profile::FromWebUI(web_ui)->GetPrefs())),
      rewards_service_(GetRewardsServiceForWebUI(web_ui))
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
      ,
      logs_handler_(rewards_service_, g_browser_process->local_state())
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)
{
  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, kAdsInternalsHost, kAdsInternalsGenerated,
      IDR_ADS_INTERNALS_HTML);
  source->AddBoolean("logsSupported", rewards_service_ != nullptr);
  source->AddBoolean("verboseLoggingEnabled", IsVerboseLoggingEnabled());
  source->AddBoolean(
      "adsInternalsVerboseModeEnabled",
      base::FeatureList::IsEnabled(brave_ads::kAdsInternalsVerboseModeFeature));
}

AdsInternalsUI::~AdsInternalsUI() = default;

void AdsInternalsUI::BindInterface(
    mojo::PendingReceiver<bat_ads::mojom::AdsInternals> pending_receiver) {
  handler_.BindInterface(std::move(pending_receiver));
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
void AdsInternalsUI::BindInterface(
    mojo::PendingReceiver<bat_ads::mojom::AdsInternalsLogs> pending_receiver) {
  logs_handler_.BindInterface(std::move(pending_receiver));
}
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

///////////////////////////////////////////////////////////////////////////////

WEB_UI_CONTROLLER_TYPE_IMPL(AdsInternalsUI)

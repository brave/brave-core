// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ads_internals/ads_internals_ui.h"

#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/browser/resources/grit/ads_internals_generated_map.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "base/feature_list.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/core/features.h"
#include "chrome/browser/browser_process.h"
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
               *Profile::FromWebUI(web_ui)->GetPrefs()),
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

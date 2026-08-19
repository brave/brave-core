// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_welcome_page/brave_welcome_page_ui.h"

#include <utility>

#include "base/check_deref.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "brave/browser/resources/brave_welcome_page/grit/brave_welcome_page_generated_map.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.h"
#include "brave/browser/ui/webui/brave_welcome_page/brave_welcome_page_prefs.h"
#include "brave/browser/ui/webui/brave_welcome_page/welcome_page_features.h"
#include "brave/browser/ui/webui/brave_welcome_page/welcome_page_handler.h"
#include "brave/browser/ui/webui/settings/brave_import_bulk_data_handler.h"
#include "brave/components/brave_education/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/background/ntp_custom_background_service_factory.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/webui/cr_components/theme_color_picker/theme_color_picker_handler.h"
#include "chrome/browser/ui/webui/settings/settings_default_browser_handler.h"
#include "components/country_codes/country_codes.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/regional_capabilities/regional_capabilities_prefs.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/webui/webui_util.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_EDUCATION)
#include "brave/browser/ui/webui/brave_education/brave_education_server_checker.h"
#include "brave/components/brave_education/features.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#endif

namespace {

inline constexpr char kBraveWelcomePageHost[] = "welcome-new";

inline constexpr char kJapanWelcomeURL[] =
    "https://brave.com/ja/desktop-ntp-tutorial";

void OpenJapanWelcomePage(base::WeakPtr<content::WebContents> web_contents) {
  if (!web_contents) {
    return;
  }
  web_contents->OpenURL(
      content::OpenURLParams(GURL(kJapanWelcomeURL), content::Referrer(),
                             WindowOpenDisposition::NEW_BACKGROUND_TAB,
                             ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false),
      /*navigation_handle_callback=*/{});
}

bool IsJapanCountryId(PrefService* prefs) {
  auto country_id = country_codes::CountryId::Deserialize(
      prefs->GetInteger(regional_capabilities::prefs::kCountryIDAtInstall));
  return country_id == country_codes::CountryId("JP");
}

}  // namespace

BraveWelcomePageUI::BraveWelcomePageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui, /*enable_chrome_send=*/true) {
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kBraveWelcomePageHost);

  webui::SetupWebUIDataSource(source, kBraveWelcomePageGenerated,
                              IDR_BRAVE_WELCOME_PAGE_HTML);

  AddBackgroundColorToSource(source, web_ui->GetWebContents());

  web_ui->AddMessageHandler(
      std::make_unique<settings::BraveImportBulkDataHandler>());
  web_ui->AddMessageHandler(
      std::make_unique<settings::DefaultBrowserHandler>());

  source->AddLocalizedStrings(webui::kBraveWelcomePageStrings);

  PrefService* local_state = g_browser_process->local_state();
  source->AddBoolean("isCrashReportingPrefManaged",
                     local_state->IsManagedPreference(
                         metrics::prefs::kMetricsReportingEnabled));
  source->AddBoolean("isP3APrefManaged",
                     local_state->IsManagedPreference(p3a::kP3AEnabled));
#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
  source->AddBoolean(
      "isWebDiscoveryPrefManaged",
      profile->GetPrefs()->IsManagedPreference(kWebDiscoveryEnabled));
#else
  source->AddBoolean("isWebDiscoveryPrefManaged", false);
#endif  // BUILDFLAG(ENABLE_WEB_DISCOVERY)
  source->AddBoolean("webDiscoveryFeatureEnabled",
                     BUILDFLAG(ENABLE_WEB_DISCOVERY));

  using brave_welcome_page::mojom::Feature;
  auto features = brave_welcome_page::GetAvailableFeatures(profile);
  source->AddBoolean("aiChatFeatureEnabled",
                     features.contains(Feature::kAIChat));
  source->AddBoolean("walletFeatureEnabled",
                     features.contains(Feature::kWallet));
  source->AddBoolean("rewardsFeatureEnabled",
                     features.contains(Feature::kRewards));
  source->AddBoolean("vpnFeatureEnabled", features.contains(Feature::kVPN));

  PrefService* prefs = profile->GetPrefs();
  if (!prefs->GetBoolean(brave_welcome_page::prefs::kHasSeenBraveWelcomePage) &&
      IsJapanCountryId(prefs)) {
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&OpenJapanWelcomePage,
                       web_ui->GetWebContents()->GetWeakPtr()),
        base::Seconds(3));
  }
  prefs->SetBoolean(brave_welcome_page::prefs::kHasSeenBraveWelcomePage, true);
}

BraveWelcomePageUI::~BraveWelcomePageUI() = default;

void BraveWelcomePageUI::BindInterface(
    mojo::PendingReceiver<brave_welcome_page::mojom::WelcomePageHandler>
        receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  page_handler_ = std::make_unique<brave_welcome_page::WelcomePageHandler>(
      std::move(receiver), brave_welcome_page::GetAvailableFeatures(profile),
      ThemeServiceFactory::GetForProfile(profile), profile->GetPrefs(),
      g_browser_process->local_state());

#if BUILDFLAG(ENABLE_BRAVE_EDUCATION)
  if (base::FeatureList::IsEnabled(
          brave_education::features::kShowGettingStartedPage)) {
    page_handler_->SetEducationServerChecker(
        std::make_unique<brave_education::BraveEducationServerChecker>(
            CHECK_DEREF(profile->GetPrefs()), profile->GetURLLoaderFactory()));
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_EDUCATION)
}

void BraveWelcomePageUI::BindInterface(
    mojo::PendingReceiver<
        theme_color_picker::mojom::ThemeColorPickerHandlerFactory> receiver) {
  theme_color_picker_handler_factory_receiver_.reset();
  theme_color_picker_handler_factory_receiver_.Bind(std::move(receiver));
}

void BraveWelcomePageUI::CreateThemeColorPickerHandler(
    mojo::PendingRemote<theme_color_picker::mojom::ThemeColorPickerClient>
        client,
    mojo::PendingReceiver<theme_color_picker::mojom::ThemeColorPickerHandler>
        handler) {
  auto* profile = Profile::FromWebUI(web_ui());
  theme_color_picker_handler_ = std::make_unique<ThemeColorPickerHandler>(
      std::move(handler), std::move(client),
      NtpCustomBackgroundServiceFactory::GetForProfile(profile),
      web_ui()->GetWebContents());
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveWelcomePageUI)

BraveWelcomePageUIConfig::BraveWelcomePageUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveWelcomePageHost) {}

bool BraveWelcomePageUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  auto* profile = Profile::FromBrowserContext(browser_context);
  return !profile->IsGuestSession();
}

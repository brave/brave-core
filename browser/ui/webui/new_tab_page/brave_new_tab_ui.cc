// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/ntp_background/brave_ntp_custom_background_service_factory.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_page_handler.h"
#include "brave/browser/ui/webui/new_tab_page/top_sites_message_handler.h"
#include "brave/components/brave_new_tab_ui/grit/brave_new_tab_ui_generated_map.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/misc_metrics/new_tab_metrics.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_images_source.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/themes/theme_syncable_service.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/resources/cr_components/searchbox/searchbox.mojom.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#endif

using ntp_background_images::NTPCustomImagesSource;

BraveNewTabUI::BraveNewTabUI(content::WebUI* web_ui, const std::string& name)
    : ui::MojoWebUIController(
          web_ui,
          true /* Needed for legacy non-mojom message handler */),
      page_factory_receiver_(this) {
  content::WebContents* web_contents = web_ui->GetWebContents();
  CHECK(web_contents);

  content::NavigationEntry* navigation_entry =
      web_contents->GetController().GetLastCommittedEntry();
  const bool was_restored =
      navigation_entry ? navigation_entry->IsRestored() : false;

  Profile* profile = Profile::FromWebUI(web_ui);
  web_ui->OverrideTitle(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_NEW_TAB_TITLE));

  if (brave::ShouldNewTabShowBlankpage(profile)) {
    content::WebUIDataSource* source =
        content::WebUIDataSource::CreateAndAdd(profile, name);
    source->SetDefaultResource(IDR_BRAVE_BLANK_NEW_TAB_HTML);
    AddBackgroundColorToSource(source, web_contents);
    return;
  }

  // Non blank NTP.
  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, name, kBraveNewTabUiGenerated, kBraveNewTabUiGeneratedSize,
      IDR_BRAVE_NEW_TAB_HTML);

  AddBackgroundColorToSource(source, web_contents);

  // Lottie animations tick on a worker thread and requires the document CSP to
  // be set to "worker-src blob: 'self';".
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc,
      "worker-src blob: chrome://resources 'self';");

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes,
      "trusted-types static-types lottie-worker-script-loader lit-html-desktop "
      "default; ");

  source->AddBoolean(
      "featureCustomBackgroundEnabled",
      !profile->GetPrefs()->IsManagedPreference(GetThemePrefNameInMigration(
          ThemePrefInMigration::kNtpCustomBackgroundDict)));

  // Let frontend know about feature flags
  source->AddBoolean("featureFlagBraveNewsPromptEnabled",
                     base::FeatureList::IsEnabled(
                         brave_news::features::kBraveNewsCardPeekFeature));

  source->AddBoolean(
      "featureFlagBraveNewsFeedV2Enabled",
      base::FeatureList::IsEnabled(brave_news::features::kBraveNewsFeedUpdate));

  source->AddBoolean(
      "featureFlagSearchWidget",
      base::FeatureList::IsEnabled(features::kBraveNtpSearchWidget));

  source->AddBoolean("vpnWidgetSupported",
#if BUILDFLAG(ENABLE_BRAVE_VPN)
                     brave_vpn::IsBraveVPNEnabled(profile->GetPrefs())
#else
                     false
#endif
  );

  web_ui->AddMessageHandler(base::WrapUnique(
      BraveNewTabMessageHandler::Create(source, profile, was_restored)));
  web_ui->AddMessageHandler(
      base::WrapUnique(new TopSitesMessageHandler(profile)));

  // For custom background images.
  if (auto* ntp_custom_background_images_service =
          BraveNTPCustomBackgroundServiceFactory::GetForContext(profile)) {
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
  // Wire up JS mojom to service
  auto* brave_news_controller =
      brave_news::BraveNewsControllerFactory::GetForBrowserContext(profile);
  if (brave_news_controller) {
    brave_news_controller->Bind(std::move(receiver));
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

void BraveNewTabUI::BindInterface(
    mojo::PendingReceiver<searchbox::mojom::PageHandler> pending_page_handler) {
  auto* profile = Profile::FromWebUI(web_ui());
  DCHECK(profile);

  realbox_handler_ = std::make_unique<RealboxHandler>(
      std::move(pending_page_handler), profile, web_ui()->GetWebContents(),
      /*metrics_reporter=*/nullptr, /*lens_searchbox_client=*/nullptr,
      /*omnibox_controller=*/nullptr);
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
void BraveNewTabUI::BindInterface(
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler>
        pending_vpn_service_handler) {
  auto* profile = Profile::FromWebUI(web_ui());
  CHECK(profile);
  auto* vpn_service = brave_vpn::BraveVpnServiceFactory::GetForProfile(profile);
  if (vpn_service) {
    vpn_service->BindInterface(std::move(pending_vpn_service_handler));
  }
}
#endif

void BraveNewTabUI::CreatePageHandler(
    mojo::PendingRemote<brave_new_tab_page::mojom::Page> pending_page,
    mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandler>
        pending_page_handler,
    mojo::PendingReceiver<brave_new_tab_page::mojom::NewTabMetrics>
        pending_new_tab_metrics) {
  DCHECK(pending_page.is_valid());
  Profile* profile = Profile::FromWebUI(web_ui());
  page_handler_ = std::make_unique<BraveNewTabPageHandler>(
      std::move(pending_page_handler), std::move(pending_page), profile,
      web_ui()->GetWebContents());
  g_brave_browser_process->process_misc_metrics()->new_tab_metrics()->Bind(
      std::move(pending_new_tab_metrics));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewTabUI)

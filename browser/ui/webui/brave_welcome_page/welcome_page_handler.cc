// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_welcome_page/welcome_page_handler.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/components/brave_education/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/common/webui_url_constants.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_EDUCATION)
#include "brave/components/brave_education/education_urls.h"
#include "brave/components/brave_education/features.h"
#endif

namespace brave_welcome_page {

namespace {

mojom::ColorScheme ToColorScheme(
    ThemeService::BrowserColorScheme browser_color_scheme) {
  switch (browser_color_scheme) {
    case ThemeService::BrowserColorScheme::kSystem:
      return mojom::ColorScheme::kSystem;
    case ThemeService::BrowserColorScheme::kLight:
      return mojom::ColorScheme::kLight;
    case ThemeService::BrowserColorScheme::kDark:
      return mojom::ColorScheme::kDark;
  }
}

ThemeService::BrowserColorScheme ToBrowserColorScheme(
    mojom::ColorScheme color_scheme) {
  switch (color_scheme) {
    case mojom::ColorScheme::kSystem:
      return ThemeService::BrowserColorScheme::kSystem;
    case mojom::ColorScheme::kLight:
      return ThemeService::BrowserColorScheme::kLight;
    case mojom::ColorScheme::kDark:
      return ThemeService::BrowserColorScheme::kDark;
  }
}

}  // namespace

WelcomePageHandler::WelcomePageHandler(
    mojo::PendingReceiver<mojom::WelcomePageHandler> receiver,
    ThemeService* theme_service,
    PrefService* prefs,
    PrefService* local_state,
    [[maybe_unused]] scoped_refptr<network::SharedURLLoaderFactory>
        url_loader_factory)
    : receiver_(this, std::move(receiver)),
      local_state_(CHECK_DEREF(local_state))
#if BUILDFLAG(ENABLE_BRAVE_EDUCATION)
      ,
      brave_education_server_checker_(CHECK_DEREF(prefs),
                                      std::move(url_loader_factory))
#endif
{
  CHECK(theme_service);
  CHECK(prefs);
  theme_service_observation_.Observe(theme_service);

  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      brave_tabs::kVerticalTabsEnabled,
      base::BindRepeating(&WelcomePageHandler::OnVerticalTabsEnabledChanged,
                          base::Unretained(this)));
}

WelcomePageHandler::~WelcomePageHandler() = default;

void WelcomePageHandler::SetWelcomePage(
    mojo::PendingRemote<mojom::WelcomePage> page) {
  page_.Bind(std::move(page));
}

void WelcomePageHandler::GetColorScheme(GetColorSchemeCallback callback) {
  std::move(callback).Run(ToColorScheme(
      theme_service_observation_.GetSource()->GetBrowserColorScheme()));
}

void WelcomePageHandler::SetColorScheme(mojom::ColorScheme color_scheme,
                                        SetColorSchemeCallback callback) {
  theme_service_observation_.GetSource()->SetBrowserColorScheme(
      ToBrowserColorScheme(color_scheme));
  std::move(callback).Run();
}

void WelcomePageHandler::GetVerticalTabsEnabled(
    GetVerticalTabsEnabledCallback callback) {
  std::move(callback).Run(pref_change_registrar_.prefs()->GetBoolean(
      brave_tabs::kVerticalTabsEnabled));
}

void WelcomePageHandler::SetVerticalTabsEnabled(
    bool enabled,
    SetVerticalTabsEnabledCallback callback) {
  pref_change_registrar_.prefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled,
                                             enabled);
  std::move(callback).Run();
}

void WelcomePageHandler::SetWebDiscoveryEnabled(
    [[maybe_unused]] bool enabled,
    SetWebDiscoveryEnabledCallback callback) {
#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
  pref_change_registrar_.prefs()->SetBoolean(kWebDiscoveryEnabled, enabled);
#endif
  std::move(callback).Run();
}

void WelcomePageHandler::SetP3AEnabled(bool enabled,
                                       SetP3AEnabledCallback callback) {
  local_state_->SetBoolean(p3a::kP3AEnabled, enabled);
  std::move(callback).Run();
}

void WelcomePageHandler::SetCrashReportsEnabled(
    bool enabled,
    SetCrashReportsEnabledCallback callback) {
  ChangeMetricsReportingState(
      enabled, metrics::ChangeMetricsReportingStateCalledFrom::kUiSettings);
  std::move(callback).Run();
}

void WelcomePageHandler::GetWelcomeCompleteURL(
    GetWelcomeCompleteURLCallback callback) {
#if BUILDFLAG(ENABLE_BRAVE_EDUCATION)
  if (base::FeatureList::IsEnabled(
          brave_education::features::kShowGettingStartedPage)) {
    brave_education_server_checker_.IsServerPageAvailable(
        brave_education::EducationPageType::kGettingStarted,
        base::BindOnce(&WelcomePageHandler::OnGettingStartedServerCheck,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }
#endif
  std::move(callback).Run(chrome::kChromeUINewTabURL);
}

#if BUILDFLAG(ENABLE_BRAVE_EDUCATION)
void WelcomePageHandler::OnGettingStartedServerCheck(
    GetWelcomeCompleteURLCallback callback,
    bool available) {
  GURL url = available
                 ? brave_education::GetEducationPageBrowserURL(
                       brave_education::EducationPageType::kGettingStarted)
                 : GURL(chrome::kChromeUINewTabURL);
  std::move(callback).Run(url.spec());
}
#endif

void WelcomePageHandler::OnThemeChanged() {
  if (page_) {
    page_->OnThemeChanged();
  }
}

void WelcomePageHandler::OnVerticalTabsEnabledChanged() {
  if (page_) {
    page_->OnVerticalTabsEnabledChanged();
  }
}

}  // namespace brave_welcome_page

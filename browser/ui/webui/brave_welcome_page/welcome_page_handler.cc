// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_welcome_page/welcome_page_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/themes/theme_service.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"

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
    PrefService* local_state)
    : receiver_(this, std::move(receiver)) {
  CHECK(theme_service);
  CHECK(prefs);
  CHECK(local_state);
  theme_service_observation_.Observe(theme_service);

  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      brave_tabs::kVerticalTabsEnabled,
      base::BindRepeating(&WelcomePageHandler::OnVerticalTabsEnabledChanged,
                          base::Unretained(this)));
#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
  pref_change_registrar_.Add(
      kWebDiscoveryEnabled,
      base::BindRepeating(&WelcomePageHandler::OnWebDiscoveryEnabledChanged,
                          base::Unretained(this)));
#endif

  local_state_change_registrar_.Init(local_state);
  local_state_change_registrar_.Add(
      p3a::kP3AEnabled,
      base::BindRepeating(&WelcomePageHandler::OnP3AEnabledChanged,
                          base::Unretained(this)));
  local_state_change_registrar_.Add(
      metrics::prefs::kMetricsReportingEnabled,
      base::BindRepeating(&WelcomePageHandler::OnCrashReportsEnabledChanged,
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

void WelcomePageHandler::GetWebDiscoveryEnabled(
    GetWebDiscoveryEnabledCallback callback) {
#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
  std::move(callback).Run(
      pref_change_registrar_.prefs()->GetBoolean(kWebDiscoveryEnabled));
#else
  std::move(callback).Run(false);
#endif
}

void WelcomePageHandler::SetWebDiscoveryEnabled(
    [[maybe_unused]] bool enabled,
    SetWebDiscoveryEnabledCallback callback) {
#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
  pref_change_registrar_.prefs()->SetBoolean(kWebDiscoveryEnabled, enabled);
#endif
  std::move(callback).Run();
}

void WelcomePageHandler::GetP3AEnabled(GetP3AEnabledCallback callback) {
  std::move(callback).Run(
      local_state_change_registrar_.prefs()->GetBoolean(p3a::kP3AEnabled));
}

void WelcomePageHandler::SetP3AEnabled(bool enabled,
                                       SetP3AEnabledCallback callback) {
  local_state_change_registrar_.prefs()->SetBoolean(p3a::kP3AEnabled, enabled);
  std::move(callback).Run();
}

void WelcomePageHandler::GetCrashReportsEnabled(
    GetCrashReportsEnabledCallback callback) {
  std::move(callback).Run(local_state_change_registrar_.prefs()->GetBoolean(
      metrics::prefs::kMetricsReportingEnabled));
}

void WelcomePageHandler::SetCrashReportsEnabled(
    bool enabled,
    SetCrashReportsEnabledCallback callback) {
  ChangeMetricsReportingState(
      enabled, metrics::ChangeMetricsReportingStateCalledFrom::kUiSettings);
  std::move(callback).Run();
}

void WelcomePageHandler::OnThemeChanged() {
  if (page_) {
    page_->OnColorSchemeChanged();
  }
}

void WelcomePageHandler::OnVerticalTabsEnabledChanged() {
  if (page_) {
    page_->OnVerticalTabsEnabledChanged();
  }
}

void WelcomePageHandler::OnWebDiscoveryEnabledChanged() {
  if (page_) {
    page_->OnWebDiscoveryEnabledChanged();
  }
}

void WelcomePageHandler::OnP3AEnabledChanged() {
  if (page_) {
    page_->OnP3AEnabledChanged();
  }
}

void WelcomePageHandler::OnCrashReportsEnabledChanged() {
  if (page_) {
    page_->OnCrashReportsEnabledChanged();
  }
}

}  // namespace brave_welcome_page

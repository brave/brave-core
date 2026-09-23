// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_welcome_page/welcome_page_handler.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/webui/brave_welcome_page/welcome_page_features.h"
#include "brave/components/brave_education/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/common/webui_url_constants.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_EDUCATION)
#include "brave/browser/ui/webui/brave_education/brave_education_server_checker.h"
#include "brave/components/brave_education/education_urls.h"
#endif

namespace brave_welcome_page {

namespace {

constexpr char kOnboardingHistogramName[] = "Brave.Welcome.InteractionStatus.2";
constexpr int kOnboardingHistogramBucketCount = 4;

// What was the last screen that you viewed during the browser onboarding
// process?
// 0. Only viewed the welcome screen, performed no action
// 1. Viewed the profile import screen
// 2. Viewed the diagnostic/analytics consent screen
// 3. Finished the onboarding process
int ToHistogramValue(mojom::OnboardingPhase phase) {
  switch (phase) {
    case mojom::OnboardingPhase::kWelcome:
      return 0;
    case mojom::OnboardingPhase::kImport:
      return 1;
    case mojom::OnboardingPhase::kMetrics:
      return 2;
    case mojom::OnboardingPhase::kFinished:
      return 3;
  }
}

void RecordOnboardingPhase(mojom::OnboardingPhase phase) {
  UMA_HISTOGRAM_EXACT_LINEAR(kOnboardingHistogramName, ToHistogramValue(phase),
                             kOnboardingHistogramBucketCount);
}

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
    const base::flat_set<mojom::Feature>& available_features,
    ThemeService* theme_service,
    PrefService* prefs,
    PrefService* local_state)
    : receiver_(this, std::move(receiver)),
      local_state_(CHECK_DEREF(local_state)) {
  CHECK(theme_service);
  CHECK(prefs);
  theme_service_observation_.Observe(theme_service);

  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      brave_tabs::kVerticalTabsEnabled,
      base::BindRepeating(&WelcomePageHandler::OnVerticalTabsEnabledChanged,
                          base::Unretained(this)));

  for (auto feature : available_features) {
    for (std::string_view pref_name : GetFeatureVisibilityPrefs(feature)) {
      feature_visibility_prefs_[feature].push_back(pref_name);
      pref_change_registrar_.Add(
          pref_name,
          base::BindRepeating(&WelcomePageHandler::OnFeatureVisibilityChanged,
                              base::Unretained(this)));
    }
  }
}

WelcomePageHandler::~WelcomePageHandler() {
  RecordOnboardingPhase(onboarding_phase_);
}

#if BUILDFLAG(ENABLE_BRAVE_EDUCATION)
void WelcomePageHandler::SetEducationServerChecker(
    std::unique_ptr<brave_education::BraveEducationServerChecker> checker) {
  education_server_checker_ = std::move(checker);
}
#endif

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

void WelcomePageHandler::GetFeatureVisibility(
    GetFeatureVisibilityCallback callback) {
  auto visibility = mojom::FeatureVisibility::New();
  visibility->ai_chat = IsFeatureVisible(mojom::Feature::kAIChat);
  visibility->wallet = IsFeatureVisible(mojom::Feature::kWallet);
  visibility->rewards = IsFeatureVisible(mojom::Feature::kRewards);
  visibility->vpn = IsFeatureVisible(mojom::Feature::kVPN);
  std::move(callback).Run(std::move(visibility));
}

void WelcomePageHandler::SetFeatureVisible(mojom::Feature feature,
                                           bool visible,
                                           SetFeatureVisibleCallback callback) {
  auto iter = feature_visibility_prefs_.find(feature);
  if (iter != feature_visibility_prefs_.end()) {
    for (std::string_view pref_name : iter->second) {
      pref_change_registrar_.prefs()->SetBoolean(pref_name, visible);
    }
  }
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
  if (education_server_checker_) {
    education_server_checker_->IsServerPageAvailable(
        brave_education::EducationPageType::kGettingStarted,
        base::BindOnce(&WelcomePageHandler::OnGettingStartedServerCheck,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_EDUCATION)
  std::move(callback).Run(chrome::kChromeUINewTabURL);
}

void WelcomePageHandler::SetOnboardingPhase(mojom::OnboardingPhase phase) {
  onboarding_phase_ = std::max(onboarding_phase_, phase);
  RecordOnboardingPhase(onboarding_phase_);
}

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

void WelcomePageHandler::OnFeatureVisibilityChanged() {
  if (page_) {
    page_->OnFeatureVisibilityChanged();
  }
}

bool WelcomePageHandler::IsFeatureVisible(mojom::Feature feature) const {
  auto iter = feature_visibility_prefs_.find(feature);
  if (iter == feature_visibility_prefs_.end()) {
    return false;
  }
  auto* prefs = pref_change_registrar_.prefs();
  return std::ranges::any_of(iter->second, [prefs](std::string_view pref_name) {
    return prefs->GetBoolean(pref_name);
  });
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
#endif  // BUILDFLAG(ENABLE_BRAVE_EDUCATION)

}  // namespace brave_welcome_page

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/onboarding/onboarding_tab_helper.h"

#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "brave/browser/onboarding/domain_map.h"
#include "brave/browser/onboarding/pref_names.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

// static
void OnboardingTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  bool is_shields_highlighted = g_browser_process->local_state()->GetBoolean(
      onboarding::prefs::kOnboardingIsShieldsHighlighted);

  if (is_shields_highlighted) {
    return;
  }

  OnboardingTabHelper::CreateForWebContents(web_contents);
}

OnboardingTabHelper::~OnboardingTabHelper() {
  Observe(nullptr);
  permission_request_manager_ = nullptr;
  timer_delay_.Stop();
}

OnboardingTabHelper::OnboardingTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<OnboardingTabHelper>(*web_contents) {
  permission_request_manager_ =
      permissions::PermissionRequestManager::FromWebContents(web_contents);
  permission_request_manager_->AddObserver(this);
}

void OnboardingTabHelper::DidStopLoading() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  DCHECK(browser);

  auto* active_tab_web_contents =
      browser->tab_strip_model()->GetActiveWebContents();

  if (web_contents() != active_tab_web_contents) {
    return;
  }

  // Avoid concurrent execution of permissions bubble checks and shields checks
  // to prevent displaying two active bubbles on the screen.
  // Add a delay to increase the likelihood of not interfering with the checks.
  timer_delay_.Start(
      FROM_HERE, base::Seconds(1), this,
      &OnboardingTabHelper::PerformBraveShieldsChecksAndShowHelpBubble);
}

void OnboardingTabHelper::WebContentsDestroyed() {
  permission_request_manager_->RemoveObserver(this);
  timer_delay_.Reset();
}

void OnboardingTabHelper::OnPromptAdded() {
  browser_has_active_bubble_ = true;
}

void OnboardingTabHelper::OnPromptRemoved() {
  browser_has_active_bubble_ = false;
}

void OnboardingTabHelper::PerformBraveShieldsChecksAndShowHelpBubble() {
  if (browser_has_active_bubble_) {
    return;
  }

  auto* shields_data_controller =
      brave_shields::BraveShieldsDataController::FromWebContents(
          web_contents());
  DCHECK(shields_data_controller);

  if (shields_data_controller->GetBraveShieldsEnabled() &&
      shields_data_controller->GetTotalBlockedCount() > 1 &&
      CanHighlightBraveShields()) {
    ShowBraveHelpBubbleView();
  }
}

bool OnboardingTabHelper::CanHighlightBraveShields() {
  bool is_shields_highlighted = g_browser_process->local_state()->GetBoolean(
      onboarding::prefs::kOnboardingIsShieldsHighlighted);

  if (is_shields_highlighted) {
    return false;
  }

  base::ScopedAllowBlockingForTesting allow_blocking;
  base::Time time_first_run = first_run::GetFirstRunSentinelCreationTime();
  base::Time time_now = base::Time::Now();
  base::Time time_7_days_ago = time_now - base::Days(7);
  bool has_browser_been_run_recently = (time_first_run > time_7_days_ago);

  if (first_run::IsChromeFirstRun() || has_browser_been_run_recently) {
    return true;
  }

  return false;
}

void OnboardingTabHelper::ShowBraveHelpBubbleView() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  DCHECK(browser);

  BraveBrowserWindow::From(browser->window())
      ->ShowBraveHelpBubbleView(GetTextForOnboardingShieldsBubble());

  g_browser_process->local_state()->SetBoolean(
      onboarding::prefs::kOnboardingIsShieldsHighlighted, true);

  Observe(nullptr);
}

std::string OnboardingTabHelper::GetTextForOnboardingShieldsBubble() {
  auto* shields_data_controller =
      brave_shields::BraveShieldsDataController::FromWebContents(
          web_contents());

  if (!shields_data_controller) {
    return std::string();
  }

  std::vector<std::string> replacements;
  auto [company_names, total_companies_blocked] =
      onboarding::GetCompanyNamesAndCountFromAdsList(
          shields_data_controller->GetBlockedAdsList());

  std::string label_text = l10n_util::GetStringUTF8(
      company_names.empty()
          ? IDS_BRAVE_SHIELDS_ONBOARDING_LABEL_WITHOUT_COMPANIES
          : IDS_BRAVE_SHIELDS_ONBOARDING_LABEL_WITH_COMPANIES);

  if (!company_names.empty()) {
    replacements.push_back(company_names);
  }

  replacements.push_back(
      base::NumberToString(shields_data_controller->GetTotalBlockedCount() -
                           total_companies_blocked));
  replacements.push_back(shields_data_controller->GetCurrentSiteURL().host());

  return base::ReplaceStringPlaceholders(label_text, replacements, nullptr);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(OnboardingTabHelper);

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
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

namespace onboarding {

OnboardingTabHelper::~OnboardingTabHelper() = default;

OnboardingTabHelper::OnboardingTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<OnboardingTabHelper>(*web_contents) {}

void OnboardingTabHelper::DidStopLoading() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  auto* shields_data_controller =
      brave_shields::BraveShieldsDataController::FromWebContents(
          web_contents());
  auto* active_tab_web_contents =
      browser->tab_strip_model()->GetActiveWebContents();

  if (web_contents() != active_tab_web_contents)
    return;

  if (shields_data_controller &&
      shields_data_controller->GetBraveShieldsEnabled() &&
      shields_data_controller->GetTotalBlockedCount() > 1 &&
      CanHighlightBraveShields()) {
    ShowBraveHelpBubbleView();
  }
}

void OnboardingTabHelper::ShowBraveHelpBubbleView() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  BraveBrowserView* browser_view = static_cast<BraveBrowserView*>(
      BraveBrowserView::GetBrowserViewForBrowser(browser));
  browser_view->ShowBraveHelpBubbleView(GetTextForOnboardingShieldsBubble());

  g_browser_process->local_state()->SetBoolean(
      prefs::kOnboardingIsShieldsHighlighted, true);
}

bool OnboardingTabHelper::CanHighlightBraveShields() {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::Time time_first_run = first_run::GetFirstRunSentinelCreationTime();
  base::Time time_now = base::Time::Now();
  base::Time time_7_days_ago = time_now - base::Days(7);
  bool has_browser_been_run_recently = (time_first_run > time_7_days_ago);
  bool is_shields_highlighted = g_browser_process->local_state()->GetBoolean(
      prefs::kOnboardingIsShieldsHighlighted);

  // TODO(nullhook): Fix this
  if (has_browser_been_run_recently && !is_shields_highlighted) {
    return true;
  }

  return false;
}

std::u16string OnboardingTabHelper::GetTextForOnboardingShieldsBubble() {
  auto* shields_data_controller =
      brave_shields::BraveShieldsDataController::FromWebContents(
          web_contents());

  if (!shields_data_controller)
    return std::u16string();

  auto blocked_ads_urls = shields_data_controller->GetBlockedAdsList();

  std::u16string label_with_companies = l10n_util::GetStringUTF16(
      IDS_BRAVE_SHIELDS_ONBOARDING_LABEL_WITH_COMPANIES);
  std::u16string label_with_out_companies = l10n_util::GetStringUTF16(
      IDS_BRAVE_SHIELDS_ONBOARDING_LABEL_WITHOUT_COMPANIES);

  std::vector<std::u16string> replacements;
  auto [company_names, total_companies_blocked] =
      DomainMap::GetCompanyNamesAndCountFromAdsList(blocked_ads_urls);

  if (!company_names.empty()) {
    replacements.push_back(company_names);
  }

  replacements.push_back(
      base::NumberToString16(shields_data_controller->GetTotalBlockedCount() -
                             total_companies_blocked));
  replacements.push_back(
      base::UTF8ToUTF16(shields_data_controller->GetCurrentSiteURL().host()));

  return base::ReplaceStringPlaceholders(
      company_names.empty() ? label_with_out_companies : label_with_companies,
      replacements, nullptr);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(OnboardingTabHelper);

}  // namespace onboarding

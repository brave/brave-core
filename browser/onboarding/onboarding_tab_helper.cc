/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/onboarding/onboarding_tab_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/browser/onboarding/pref_names.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

namespace onboarding {
void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(prefs::kLastShieldsIconHighlightTime,
                             base::Time());
}
}  // namespace onboarding

namespace {

OnboardingTabHelper::WebContentsCallback& CanHighlightCallbackForTesting() {
  static base::NoDestructor<OnboardingTabHelper::WebContentsCallback> callback;
  return *callback;
}

OnboardingTabHelper::WebContentsCallback& DidStopLoadingCallbackForTesting() {
  static base::NoDestructor<OnboardingTabHelper::WebContentsCallback> callback;
  return *callback;
}

}  // namespace

// static
bool OnboardingTabHelper::IsSevenDaysPassedSinceFirstRun() {
  base::Time time_first_run = first_run::GetFirstRunSentinelCreationTime();
  base::Time time_7_days_ago = base::Time::Now() - base::Days(7);
  return time_first_run < time_7_days_ago;
}

// static
std::unique_ptr<OnboardingTabHelper> OnboardingTabHelper::MaybeCreate(
    tabs::TabInterface& tab) {
  if (!g_browser_process->local_state()) {
    CHECK_IS_TEST();
    return nullptr;
  }

  base::Time last_shields_icon_highlight_time =
      g_browser_process->local_state()->GetTime(
          onboarding::prefs::kLastShieldsIconHighlightTime);

  // Shields highlight is aleady shown. We use it only once.
  if (!last_shields_icon_highlight_time.is_null()) {
    return nullptr;
  }

  auto helper = std::make_unique<OnboardingTabHelper>(tab);
  if (first_run::IsChromeFirstRun()) {
    helper->SetCanHighlight();
    return helper;
  }

  // Don't show highlight if already 7 days passed for non first run.
  // OnboardingTabHelper::IsSevenDaysPassedSinceFirstRun() could have IO.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&OnboardingTabHelper::IsSevenDaysPassedSinceFirstRun),
      base::BindOnce(
          &OnboardingTabHelper::OnSevenDaysPassedSinceFirstRunChecked,
          helper->weak_factory_.GetWeakPtr()));
  return helper;
}

OnboardingTabHelper::OnboardingTabHelper(tabs::TabInterface& tab)
    : tabs::ContentsObservingTabFeature(tab) {}

OnboardingTabHelper::~OnboardingTabHelper() = default;

void OnboardingTabHelper::OnSevenDaysPassedSinceFirstRunChecked(bool passed) {
  if (passed) {
    CleanUp();
    return;
  }
  SetCanHighlight();
}

void OnboardingTabHelper::SetCanHighlight() {
  can_highlight_ = true;
  if (CanHighlightCallbackForTesting()) {
    CHECK_IS_TEST();
    CanHighlightCallbackForTesting().Run(web_contents());
  }
}

// static
base::AutoReset<OnboardingTabHelper::WebContentsCallback>
OnboardingTabHelper::SetCanHighlightCallbackForTesting(
    WebContentsCallback callback) {
  return base::AutoReset<WebContentsCallback>(&CanHighlightCallbackForTesting(),
                                              std::move(callback));
}

// static
base::AutoReset<OnboardingTabHelper::WebContentsCallback>
OnboardingTabHelper::SetDidStopLoadingCallbackForTesting(
    WebContentsCallback callback) {
  return base::AutoReset<WebContentsCallback>(
      &DidStopLoadingCallbackForTesting(), std::move(callback));
}

void OnboardingTabHelper::OnDiscardContents(
    tabs::TabInterface* tab,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  tabs::ContentsObservingTabFeature::OnDiscardContents(tab, old_contents,
                                                       new_contents);
  if (cleaned_up_) {
    Observe(nullptr);
  }
}

void OnboardingTabHelper::DidStopLoading() {
  // Showing the highlight stops observing the contents.
  content::WebContents* const contents = web_contents();
  MaybeHighlightShields();
  if (DidStopLoadingCallbackForTesting()) {
    CHECK_IS_TEST();
    DidStopLoadingCallbackForTesting().Run(contents);
  }
}

void OnboardingTabHelper::MaybeHighlightShields() {
  if (!can_highlight_ || !tab().IsActivated()) {
    return;
  }

  // Show highlight when there is no permission request after loaded.
  // If permission requests are existed, don't try to show highlight in this
  // loading.
  auto* permission_request_manager =
      permissions::PermissionRequestManager::FromWebContents(web_contents());

  // Can be null in unit test.
  if (!permission_request_manager) {
    return;
  }

  if (permission_request_manager->has_pending_requests() ||
      permission_request_manager->IsRequestInProgress()) {
    return;
  }

  PerformBraveShieldsChecksAndShowHelpBubble();
}

void OnboardingTabHelper::PerformBraveShieldsChecksAndShowHelpBubble() {
  auto* shields_data_controller =
      brave_shields::BraveShieldsTabHelper::From(&tab());
  DCHECK(shields_data_controller);

  if (shields_data_controller->IsBraveShieldsEnabled() &&
      shields_data_controller->GetTotalBlockedCount() > 0 &&
      CanHighlightBraveShields()) {
    ShowBraveHelpBubbleView();
  }
}

bool OnboardingTabHelper::CanHighlightBraveShields() {
  base::Time last_shields_icon_highlight_time =
      g_browser_process->local_state()->GetTime(
          onboarding::prefs::kLastShieldsIconHighlightTime);

  // If highlight is shown from other tabs after this tab is created,
  // this condition could be met.
  if (!last_shields_icon_highlight_time.is_null()) {
    return false;
  }

  // Show highlight for first run always if it's not shown so far.
  if (first_run::IsChromeFirstRun()) {
    return true;
  }

  // We only show highlight to users that has installed the browser in the
  // previous 7 days.
  return !IsSevenDaysPassedSinceFirstRun();
}

void OnboardingTabHelper::ShowBraveHelpBubbleView() {
  BrowserWindowInterface* browser = tab().GetBrowserWindowInterface();
  auto* brave_browser_window =
      browser ? BraveBrowserWindow::FromBrowser(browser) : nullptr;
  if (!brave_browser_window || !brave_browser_window->ShowBraveHelpBubbleView(
                                   GetTextForOnboardingShieldsBubble())) {
    return;
  }

  g_browser_process->local_state()->SetTime(
      onboarding::prefs::kLastShieldsIconHighlightTime, base::Time::Now());

  CleanUp();
}

std::string OnboardingTabHelper::GetTextForOnboardingShieldsBubble() {
  auto* shields_data_controller =
      brave_shields::BraveShieldsTabHelper::From(&tab());

  if (!shields_data_controller) {
    return std::string();
  }

  std::vector<std::string> replacements;
  std::string label_text = l10n_util::GetPluralStringFUTF8(
      IDS_BRAVE_SHIELDS_ONBOARDING_LABEL_WITHOUT_COMPANIES,
      shields_data_controller->GetTotalBlockedCount());
  replacements.push_back(
      std::string(shields_data_controller->GetCurrentSiteURL().host()));

  return base::ReplaceStringPlaceholders(label_text, replacements, nullptr);
}

void OnboardingTabHelper::CleanUp() {
  can_highlight_ = false;
  cleaned_up_ = true;
  Observe(nullptr);
}

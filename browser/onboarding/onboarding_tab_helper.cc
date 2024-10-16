/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/onboarding/onboarding_tab_helper.h"

#include <string>
#include <utility>
#include <vector>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/browser/onboarding/pref_names.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

namespace onboarding {
void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(prefs::kLastShieldsIconHighlightTime,
                             base::Time());
}
}  // namespace onboarding

// static
bool OnboardingTabHelper::IsSevenDaysPassedSinceFirstRun() {
  base::Time time_first_run = s_sentinel_time_for_testing_.value_or(
      first_run::GetFirstRunSentinelCreationTime());
  base::Time time_now = s_time_now_for_testing_.value_or(base::Time::Now());

  base::Time time_7_days_ago = time_now - base::Days(7);
  return time_first_run < time_7_days_ago;
}

// static
std::optional<base::Time> OnboardingTabHelper::s_time_now_for_testing_;
std::optional<base::Time> OnboardingTabHelper::s_sentinel_time_for_testing_;

// static
void OnboardingTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents,
    base::OnceClosure creation_callback_for_test) {
  if (!g_browser_process->local_state()) {
    CHECK_IS_TEST();
    return;
  }

  if (creation_callback_for_test) {
    CHECK_IS_TEST();
  }

  base::Time last_shields_icon_highlight_time =
      g_browser_process->local_state()->GetTime(
          onboarding::prefs::kLastShieldsIconHighlightTime);

  // Shields highlight is aleady shown. We use it only once.
  if (!last_shields_icon_highlight_time.is_null()) {
    if (creation_callback_for_test) {
      std::move(creation_callback_for_test).Run();
    }
    return;
  }

  if (first_run::IsChromeFirstRun()) {
    OnboardingTabHelper::CreateForWebContents(web_contents);
    if (creation_callback_for_test) {
      std::move(creation_callback_for_test).Run();
    }
    return;
  }

  // Don't show highlight if already 7 days passed for non first run.
  // OnboardingTabHelper::IsSevenDaysPassedSinceFirstRun() could have IO.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&OnboardingTabHelper::IsSevenDaysPassedSinceFirstRun),
      base::BindOnce(
          [](base::WeakPtr<content::WebContents> contents,
             base::OnceCallback<void()> creation_callback_for_test,
             bool passed) {
            if (!passed && contents) {
              OnboardingTabHelper::CreateForWebContents(contents.get());
            }
            if (creation_callback_for_test) {
              std::move(creation_callback_for_test).Run();
            }
          },
          web_contents->GetWeakPtr(), std::move(creation_callback_for_test)));
}

OnboardingTabHelper::OnboardingTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<OnboardingTabHelper>(*web_contents) {}

OnboardingTabHelper::~OnboardingTabHelper() = default;

void OnboardingTabHelper::DidStopLoading() {
  Browser* browser = chrome::FindBrowserWithTab(web_contents());
  DCHECK(browser);

  if (!browser) {
    return;
  }

  auto* active_tab_web_contents =
      browser->tab_strip_model()->GetActiveWebContents();

  if (web_contents() != active_tab_web_contents) {
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
      brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents());
  DCHECK(shields_data_controller);

  if (shields_data_controller->GetBraveShieldsEnabled() &&
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
  Browser* browser = chrome::FindBrowserWithTab(web_contents());
  DCHECK(browser);
  if (!browser) {
    return;
  }

  if (!BraveBrowserWindow::From(browser->window())
           ->ShowBraveHelpBubbleView(GetTextForOnboardingShieldsBubble())) {
    return;
  }

  g_browser_process->local_state()->SetTime(
      onboarding::prefs::kLastShieldsIconHighlightTime, base::Time::Now());

  CleanUp();
}

std::string OnboardingTabHelper::GetTextForOnboardingShieldsBubble() {
  auto* shields_data_controller =
      brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents());

  if (!shields_data_controller) {
    return std::string();
  }

  std::vector<std::string> replacements;
  std::string label_text = l10n_util::GetPluralStringFUTF8(
      IDS_BRAVE_SHIELDS_ONBOARDING_LABEL_WITHOUT_COMPANIES,
      shields_data_controller->GetTotalBlockedCount());
  replacements.push_back(shields_data_controller->GetCurrentSiteURL().host());

  return base::ReplaceStringPlaceholders(label_text, replacements, nullptr);
}

void OnboardingTabHelper::CleanUp() {
  Observe(nullptr);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(OnboardingTabHelper);

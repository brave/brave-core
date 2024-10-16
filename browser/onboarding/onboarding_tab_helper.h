/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_
#define BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_

#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/time/time.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class PrefRegistrySimple;

namespace onboarding {
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
}  // namespace onboarding

class OnboardingTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<OnboardingTabHelper> {
 public:
  // |creation_callback_for_test| is called in test after this
  // method determines whether tab helper is created or not.
  static void MaybeCreateForWebContents(
      content::WebContents* contents,
      base::OnceClosure creation_callback_for_test = base::NullCallback());
  OnboardingTabHelper(const OnboardingTabHelper&) = delete;
  OnboardingTabHelper& operator=(const OnboardingTabHelper&) = delete;
  ~OnboardingTabHelper() override;

 private:
  friend class content::WebContentsUserData<OnboardingTabHelper>;
  FRIEND_TEST_ALL_PREFIXES(OnboardingTest, HelperCreationTestForFirstRun);
  FRIEND_TEST_ALL_PREFIXES(OnboardingTest, HelperCreationTestForNonFirstRun);

  explicit OnboardingTabHelper(content::WebContents* web_contents);

  // content::WebContentsObserver
  void DidStopLoading() override;

  void PerformBraveShieldsChecksAndShowHelpBubble();
  bool CanHighlightBraveShields();
  void ShowBraveHelpBubbleView();
  std::string GetTextForOnboardingShieldsBubble();
  void CleanUp();

  static bool IsSevenDaysPassedSinceFirstRun();

  static std::optional<base::Time> s_time_now_for_testing_;
  static std::optional<base::Time> s_sentinel_time_for_testing_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_

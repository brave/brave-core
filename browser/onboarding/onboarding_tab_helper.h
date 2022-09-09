/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_
#define BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_

#include "brave/browser/ui/brave_shields_data_controller.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace onboarding {

class OnboardingTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<OnboardingTabHelper> {
 public:
  OnboardingTabHelper(const OnboardingTabHelper&) = delete;
  OnboardingTabHelper& operator=(const OnboardingTabHelper&) = delete;
  ~OnboardingTabHelper() override;

 private:
  friend class content::WebContentsUserData<OnboardingTabHelper>;

  explicit OnboardingTabHelper(content::WebContents* web_contents);

  // content::WebContentsObserver
  void DidStopLoading() override;

  void ShowBraveHelpBubbleView();
  bool CanHighlightBraveShields();
  std::u16string GetTextForOnboardingShieldsBubble();

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace onboarding

#endif  // BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_

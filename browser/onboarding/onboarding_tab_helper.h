/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_
#define BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"

class PrefRegistrySimple;

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace onboarding {
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
}  // namespace onboarding

// Highlights the Brave Shields icon once, for users who installed Brave in the
// last seven days. Owned by BraveTabFeatures.
class OnboardingTabHelper : public tabs::ContentsObservingTabFeature {
 public:
  // Returns nullptr when the highlight has already been shown.
  static std::unique_ptr<OnboardingTabHelper> MaybeCreate(
      tabs::TabInterface& tab);

  explicit OnboardingTabHelper(tabs::TabInterface& tab);
  OnboardingTabHelper(const OnboardingTabHelper&) = delete;
  OnboardingTabHelper& operator=(const OnboardingTabHelper&) = delete;
  ~OnboardingTabHelper() override;

 private:
  friend class OnboardingBrowserTest;
  friend class OnboardingTest;
  FRIEND_TEST_ALL_PREFIXES(OnboardingTest, HelperCreationTestForFirstRun);
  FRIEND_TEST_ALL_PREFIXES(OnboardingTest, HelperCreationTestForNonFirstRun);

  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // content::WebContentsObserver:
  void DidStopLoading() override;

  void OnSevenDaysPassedSinceFirstRunChecked(bool passed);
  void PerformBraveShieldsChecksAndShowHelpBubble();
  bool CanHighlightBraveShields();
  void ShowBraveHelpBubbleView();
  std::string GetTextForOnboardingShieldsBubble();
  void CleanUp();

  static bool IsSevenDaysPassedSinceFirstRun();

  static std::optional<base::Time> s_time_now_for_testing_;
  static std::optional<base::Time> s_sentinel_time_for_testing_;

  // Whether the highlight may be shown in this tab. False until the first-run
  // check completes, and once the helper has cleaned up.
  bool can_highlight_ = false;
  // Set once the helper no longer observes the tab's contents, including any
  // that replace them.
  bool cleaned_up_ = false;

  base::WeakPtrFactory<OnboardingTabHelper> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_

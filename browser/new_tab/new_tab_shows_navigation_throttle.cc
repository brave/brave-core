/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/new_tab_shows_navigation_throttle.h"

#include <string>

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

// static
std::unique_ptr<NewTabShowsNavigationThrottle>
NewTabShowsNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  auto* context = navigation_handle->GetWebContents()->GetBrowserContext();
  if (!brave::IsRegularProfile(context) ||
      !NewTabUI::IsNewTab(navigation_handle->GetURL()))
    return nullptr;

  return std::make_unique<NewTabShowsNavigationThrottle>(navigation_handle);
}

NewTabShowsNavigationThrottle::NewTabShowsNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : NavigationThrottle(navigation_handle) {}
NewTabShowsNavigationThrottle::~NewTabShowsNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
NewTabShowsNavigationThrottle::WillStartRequest() {
  auto* web_contents = navigation_handle()->GetWebContents();
  auto* context = web_contents->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(context);
  if (brave::ShouldUseNewTabURLForNewTab(profile)) {
    return content::NavigationThrottle::PROCEED;
  }

  new_tab_options_url_ = brave::GetNewTabPageURL(profile);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&NewTabShowsNavigationThrottle::LoadNewTabOptionsURL,
                     weak_factory_.GetWeakPtr()));
  return content::NavigationThrottle::CANCEL;
}

const char* NewTabShowsNavigationThrottle::GetNameForLogging() {
  return "NewTabShowsNavigationThrottle";
}

void NewTabShowsNavigationThrottle::LoadNewTabOptionsURL() {
  auto* web_contents = navigation_handle()->GetWebContents();
  web_contents->GetController().LoadURL(new_tab_options_url_,
                                        content::Referrer(),
                                        ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                        std::string());
}

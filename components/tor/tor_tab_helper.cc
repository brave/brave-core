/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_tab_helper.h"

#include "base/check.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"

namespace tor {

TorTabHelper::TorTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<TorTabHelper>(*web_contents) {}

TorTabHelper::~TorTabHelper() = default;

// static
void TorTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  if (!web_contents->GetBrowserContext()->IsTor()) {
    return;
  }
  TorTabHelper::CreateForWebContents(web_contents);
}

void TorTabHelper::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  const bool should_disable_web_share = BUILDFLAG(IS_MAC);

  blink::web_pref::WebPreferences prefs =
      web_contents()->GetOrCreateWebPreferences();
  if (prefs.disable_web_share != should_disable_web_share) {
    prefs.disable_web_share = should_disable_web_share;
    web_contents()->SetWebPreferences(prefs);
  }
}

void TorTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // We will keep retrying every second if we can't establish connection to tor
  // process. This is possible when tor is launched but not yet ready to accept
  // new connection or some fatal errors within tor process
  if (navigation_handle->GetNetErrorCode() !=
      net::ERR_PROXY_CONNECTION_FAILED) {
    return;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&TorTabHelper::ReloadTab, weak_ptr_factory_.GetWeakPtr(),
                     navigation_handle->GetWebContents()),
      base::Seconds(1));
}

void TorTabHelper::ReloadTab(content::WebContents* web_contents) {
  DCHECK(web_contents);
  web_contents->GetController().Reload(content::ReloadType::NORMAL, false);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(TorTabHelper);

}  // namespace tor

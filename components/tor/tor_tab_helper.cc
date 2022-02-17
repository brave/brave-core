/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_tab_helper.h"

#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/browser/navigation_handle.h"

namespace tor {

TorTabHelper::TorTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<TorTabHelper>(*web_contents) {}

TorTabHelper::~TorTabHelper() = default;

// static
void TorTabHelper::MaybeCreateForWebContents(content::WebContents* web_contents,
                                             bool is_tor_profile) {
  if (!is_tor_profile)
    return;
  TorTabHelper::CreateForWebContents(web_contents);
}

void TorTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // We will keep retrying every second if we can't establish connection to tor
  // process. This is possible when tor is launched but not yet ready to accept
  // new connection or some fatal errors within tor process
  if (navigation_handle->GetNetErrorCode() != net::ERR_PROXY_CONNECTION_FAILED)
    return;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&TorTabHelper::ReloadTab, AsWeakPtr(),
                     navigation_handle->GetWebContents()),
      base::Seconds(1));
}

void TorTabHelper::ReloadTab(content::WebContents* web_contents) {
  DCHECK(web_contents);
  web_contents->GetController().Reload(content::ReloadType::NORMAL, false);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(TorTabHelper);

}  // namespace tor

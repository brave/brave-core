/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_tab_helper.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace tor {

TorTabHelper::TorTabHelper(tabs::TabInterface& tab)
    : content::WebContentsObserver(tab.GetContents()),
      will_discard_contents_subscription_(tab.RegisterWillDiscardContents(
          base::BindRepeating(&TorTabHelper::OnDiscardContents,
                              base::Unretained(this)))) {}

TorTabHelper::~TorTabHelper() = default;

// static
std::unique_ptr<TorTabHelper> TorTabHelper::MaybeCreate(
    tabs::TabInterface& tab) {
  if (!tab.GetContents()->GetBrowserContext()->IsTor()) {
    return nullptr;
  }
  return std::make_unique<TorTabHelper>(tab);
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

void TorTabHelper::OnDiscardContents(tabs::TabInterface* tab,
                                     content::WebContents* old_contents,
                                     content::WebContents* new_contents) {
  // A pending reload is for the old contents.
  weak_ptr_factory_.InvalidateWeakPtrs();
  Observe(new_contents);
}

}  // namespace tor

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/restart_navigation.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace brave_shields {

bool RestartNavigationWithURL(content::NavigationHandle& navigation_handle,
                              const GURL& url,
                              ui::PageTransition transition) {
  content::WebContents* web_contents = navigation_handle.GetWebContents();
  if (!web_contents) {
    return false;
  }

  content::OpenURLParams params =
      content::OpenURLParams::FromNavigationHandle(&navigation_handle);
  params.url = url;
  params.transition = transition;
  // We get a DCHECK here if we don't clear the redirect chain because
  // technically this is a new navigation
  params.redirect_chain.clear();

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<content::WebContents> web_contents,
                        const content::OpenURLParams& params) {
                       if (!web_contents) {
                         return;
                       }
                       web_contents->OpenURL(params,
                                             /*navigation_handle_callback=*/{});
                     },
                     web_contents->GetWeakPtr(), std::move(params)));
  return true;
}

}  // namespace brave_shields

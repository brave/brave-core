// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/bwc_factory_util.h"

#include <memory>
#include <string_view>

#include "base/functional/bind.h"
#include "brave/components/local_ai/content/background_web_contents_impl.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "url/gurl.h"

namespace local_ai {

LocalAIServiceBase::BackgroundWebContentsFactory MakeBWCFactory(
    content::BrowserContext* context,
    std::string_view url,
    int task_manager_title_id) {
  return base::BindRepeating(
      [](content::BrowserContext* browser_context, const GURL& page_url,
         int title_id, BackgroundWebContents::Delegate* delegate)
          -> std::unique_ptr<BackgroundWebContents> {
        return std::make_unique<BackgroundWebContentsImpl>(
            browser_context, page_url, delegate,
            base::BindOnce(
                [](int id, content::WebContents* wc) {
                  task_manager::WebContentsTags::CreateForToolContents(wc, id);
                },
                title_id));
      },
      context, GURL(url), task_manager_title_id);
}

}  // namespace local_ai

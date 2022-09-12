/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_CONTENT_BROWSER_CLIENT_HELPER_H_
#define BRAVE_BROWSER_IPFS_CONTENT_BROWSER_CLIENT_HELPER_H_

#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace content {
class BrowserContext;
}  // namespace content

namespace url {
class Origin;
}  // namespace url

namespace ipfs {

bool HandleIPFSURLReverseRewrite(
    GURL* url,
    content::BrowserContext* browser_context);

void LoadOrLaunchIPFSURL(const GURL& url,
                         content::WebContents::OnceGetter web_contents_getter,
                         ui::PageTransition page_transition,
                         bool has_user_gesture,
                         const absl::optional<url::Origin>& initiating_origin);

bool HandleIPFSURLRewrite(GURL* url,
    content::BrowserContext* browser_context);

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_CONTENT_BROWSER_CLIENT_HELPER_H_

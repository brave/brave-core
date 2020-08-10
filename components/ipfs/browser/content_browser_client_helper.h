/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BROWSER_CONTENT_BROWSER_CLIENT_HELPER_H_
#define BRAVE_COMPONENTS_IPFS_BROWSER_CONTENT_BROWSER_CLIENT_HELPER_H_

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "brave/common/url_constants.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/ipfs/browser/features.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_set.h"
#include "net/base/escape.h"

namespace ipfs {

static bool TranslateIPFSURL(const GURL& url, GURL* new_url) {
  if (!url.SchemeIs(kIPFSScheme)) {
    return false;
  }

  std::string path = url.path();
  // In the case of a URL like ipfs://[cid]/wiki/Vincent_van_Gogh.html
  // host is empty and path is //wiki/Vincent_van_Gogh.html
  if (url.host().empty() && path.length() > 2 &&
      path.substr(0, 2) == "//") {
    std::string cid(path.substr(2));
    // If we have a path after the CID, get at the real resource path
    size_t pos = cid.find("/");
    std::string path;
    if (pos != std::string::npos && pos != 0) {
      // path would be /wiki/Vincent_van_Gogh.html
      path = cid.substr(pos, cid.length() - pos);
      // cid would be [cid]
      cid = cid.substr(0, pos);
    }
    if (std::all_of(cid.begin(), cid.end(),
                    [loc = std::locale{}](char c) {
                      return std::isalnum(c, loc);
                    })) {
      // new_url would be:
      // https://dweb.link/ipfs/[cid]//wiki/Vincent_van_Gogh.html
      *new_url = GURL(std::string(kDefaultIPFSGateway) + cid + path);
      return true;
    }
  }
  return false;
}

static bool HandleIPFSURLReverseRewrite(GURL* url,
    content::BrowserContext* browser_context) {
  return false;
}

static void LoadOrLaunchIPFSURL(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiating_origin) {
  content::WebContents* web_contents = std::move(web_contents_getter).Run();
  if (!web_contents)
    return;

  web_contents->GetController().LoadURL(url, content::Referrer(),
      page_transition, std::string());
  // TODO(bbondy): We probably want something more like this:
  /*
  if (IsWebtorrentEnabled(web_contents->GetBrowserContext())) {
    web_contents->GetController().LoadURL(url, content::Referrer(),
        page_transition, std::string());
  } else {
    ExternalProtocolHandler::LaunchUrl(
        url, web_contents->GetRenderViewHost()->GetProcess()->GetID(),
        web_contents->GetRenderViewHost()->GetRoutingID(), page_transition,
        has_user_gesture, initiating_origin);
  }
  */
}

static bool HandleIPFSURLRewrite(GURL* url,
    content::BrowserContext* browser_context) {
  if (url->SchemeIs(kIPFSScheme)) {
    return TranslateIPFSURL(*url, url);
  }

  return false;
}

static void HandleIPFSProtocol(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiating_origin) {
  DCHECK(url.SchemeIs(kIPFSScheme));
  base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                 base::BindOnce(&LoadOrLaunchIPFSURL, url,
                                std::move(web_contents_getter), page_transition,
                                has_user_gesture, initiating_origin));
}

static bool IsIPFSProtocol(const GURL& url) {
  if (!base::FeatureList::IsEnabled(ipfs::features::kIpfsFeature))
    return false;

  GURL new_url;
  return TranslateIPFSURL(url, &new_url);
}

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_BROWSER_CONTENT_BROWSER_CLIENT_HELPER_H_

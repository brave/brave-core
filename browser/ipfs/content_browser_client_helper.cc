/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/content_browser_client_helper.h"

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "brave/common/url_constants.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/ipfs/browser/features.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_set.h"
#include "net/base/escape.h"
#include "url/gurl.h"

namespace {

bool IsIPFSDisabled(content::BrowserContext* browser_context) {
  auto* prefs = Profile::FromBrowserContext(browser_context)->GetPrefs();
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      prefs->GetInteger(kIPFSResolveMethod));
  return resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_DISABLED;
}

bool IsIPFSLocalGateway(content::BrowserContext* browser_context) {
  auto* prefs = Profile::FromBrowserContext(browser_context)->GetPrefs();
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      prefs->GetInteger(kIPFSResolveMethod));
  return resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_LOCAL;
}

}  // namespace

namespace ipfs {

// static
bool ContentBrowserClientHelper::TranslateIPFSURL(
    const GURL& url, GURL* new_url, bool local) {
  if (!url.SchemeIs(kIPFSScheme) && !url.SchemeIs(kIPNSScheme)) {
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
    bool ipfs_scheme = url.scheme() == "ipfs";
    bool ipns_scheme = url.scheme() == "ipns";
    if ((ipfs_scheme || ipns_scheme) && std::all_of(cid.begin(), cid.end(),
                    [loc = std::locale{}](char c) {
                      return std::isalnum(c, loc);
                    })) {
      // new_url would be:
      // https://dweb.link/ipfs/[cid]//wiki/Vincent_van_Gogh.html
      if (new_url) {
        *new_url = GURL(std::string(
            local ? kDefaultIPFSLocalGateway : kDefaultIPFSGateway) +
                (ipfs_scheme ? "/ipfs/" : "/ipns/") + cid + path);
        VLOG(1) << "[IPFS] " << __func__ << " new URL: " << *new_url;
      }

      return true;
    }
  }
  return false;
}

// static
bool ContentBrowserClientHelper::HandleIPFSURLReverseRewrite(GURL* url,
    content::BrowserContext* browser_context) {
  return false;
}

// static
void ContentBrowserClientHelper::LoadOrLaunchIPFSURL(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiating_origin) {
  content::WebContents* web_contents = std::move(web_contents_getter).Run();
  if (!web_contents)
    return;

  auto* browser_context = web_contents->GetBrowserContext();
  if (!IsIPFSDisabled(browser_context)) {
    web_contents->GetController().LoadURL(url, content::Referrer(),
        page_transition, std::string());
  } else {
    ExternalProtocolHandler::LaunchUrl(
        url, web_contents->GetRenderViewHost()->GetProcess()->GetID(),
        web_contents->GetRenderViewHost()->GetRoutingID(), page_transition,
        has_user_gesture, initiating_origin);
  }
}

// static
bool ContentBrowserClientHelper::HandleIPFSURLRewrite(GURL* url,
    content::BrowserContext* browser_context) {
  if (!IsIPFSDisabled(browser_context) &&
      (url->SchemeIs(kIPFSScheme) || url->SchemeIs(kIPNSScheme))) {
    return TranslateIPFSURL(*url, url, IsIPFSLocalGateway(browser_context));
  }

  return false;
}

// static
void ContentBrowserClientHelper::HandleIPFSProtocol(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiating_origin) {
  DCHECK(url.SchemeIs(kIPFSScheme) || url.SchemeIs(kIPNSScheme));
  base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                 base::BindOnce(&LoadOrLaunchIPFSURL, url,
                                std::move(web_contents_getter), page_transition,
                                has_user_gesture, initiating_origin));
}

// static
bool ContentBrowserClientHelper::IsIPFSProtocol(const GURL& url) {
  return TranslateIPFSURL(url, nullptr, false);
}

}  // namespace ipfs

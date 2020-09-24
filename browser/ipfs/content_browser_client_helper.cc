/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/content_browser_client_helper.h"

#include <string>
#include <utility>

#include "base/task/post_task.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/url_constants.h"
#include "brave/components/ipfs/browser/ipfs_service.h"
#include "brave/components/ipfs/browser/translate_ipfs_uri.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "brave/components/ipfs/common/pref_names.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "url/gurl.h"

namespace {

bool IsIPFSDisabled(content::BrowserContext* browser_context) {
  auto* prefs = user_prefs::UserPrefs::Get(browser_context);
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      prefs->GetInteger(kIPFSResolveMethod));
  return resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_DISABLED ||
         !ipfs::IpfsService::IsIpfsEnabled(browser_context,
             brave::IsRegularProfile(browser_context));
}

bool IsIPFSLocalGateway(content::BrowserContext* browser_context) {
  auto* prefs = user_prefs::UserPrefs::Get(browser_context);
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      prefs->GetInteger(kIPFSResolveMethod));
  return resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_LOCAL;
}

}  // namespace

namespace ipfs {

// static
bool ContentBrowserClientHelper::HandleIPFSURLRewrite(GURL* url,
    content::BrowserContext* browser_context) {
  if (!IsIPFSDisabled(browser_context) &&
      // When it's not the local gateway we don't want to show a ipfs:// URL.
      // We instead will translate the URL later in LoadOrLaunchIPFSURL.
      IsIPFSLocalGateway(browser_context) &&
      (url->SchemeIs(kIPFSScheme) || url->SchemeIs(kIPNSScheme))) {
    return TranslateIPFSURI(*url, url, true);
  }

  return false;
}

// static
bool ContentBrowserClientHelper::HandleIPFSURLReverseRewrite(GURL* url,
    content::BrowserContext* browser_context) {
  return false;
}

// static
bool ContentBrowserClientHelper::ShouldNavigateIPFSURI(const GURL& url,
    GURL* new_url, content::BrowserContext* browser_context) {
  *new_url = url;
  bool is_ipfs_scheme = url.SchemeIs(kIPFSScheme) || url.SchemeIs(kIPNSScheme);
  return !IsIPFSDisabled(browser_context) && (!is_ipfs_scheme ||
      TranslateIPFSURI(url, new_url, IsIPFSLocalGateway(browser_context)));
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
  GURL new_url(url);
  if (ShouldNavigateIPFSURI(url, &new_url, web_contents->GetBrowserContext())) {
    web_contents->GetController().LoadURL(new_url, content::Referrer(),
        page_transition, std::string());
  } else {
    ExternalProtocolHandler::LaunchUrl(
        new_url, web_contents->GetRenderViewHost()->GetProcess()->GetID(),
        web_contents->GetRenderViewHost()->GetRoutingID(), page_transition,
        has_user_gesture, initiating_origin);
  }
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
  return TranslateIPFSURI(url, nullptr, false);
}

}  // namespace ipfs

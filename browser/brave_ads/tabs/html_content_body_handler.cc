/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/html_content_body_handler.h"

#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"
#include "brave/browser/brave_ads/tabs/tabs_util.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_type.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

namespace brave_ads {

namespace {

content::BrowserContext* GetBrowserContext(
    const content::WebContents::Getter& web_contents_getter) {
  content::WebContents* const web_contents = web_contents_getter.Run();
  if (!web_contents) {
    return nullptr;
  }

  return web_contents->GetBrowserContext();
}

bool IsInterstitialOrErrorPageNavigation(
    content::NavigationEntry* navigation_entry) {
  CHECK(navigation_entry);
  return navigation_entry->GetPageType() == content::PAGE_TYPE_ERROR;
}

}  // namespace

HtmlContentBodyHandler::HtmlContentBodyHandler(
    AdsService* const ads_service,
    content::WebContents::Getter web_contents_getter)
    : ads_service_(ads_service),
      web_contents_getter_(std::move(web_contents_getter)) {
  CHECK(ads_service_);
}

HtmlContentBodyHandler::~HtmlContentBodyHandler() = default;

// static
std::unique_ptr<HtmlContentBodyHandler> HtmlContentBodyHandler::MaybeCreate(
    AdsService* const ads_service,
    const content::WebContents::Getter& web_contents_getter) {
  content::BrowserContext* const browser_context =
      GetBrowserContext(web_contents_getter);
  if (!browser_context || browser_context->IsOffTheRecord()) {
    return {};
  }

  return base::WrapUnique(
      new HtmlContentBodyHandler(ads_service, web_contents_getter));
}

bool HtmlContentBodyHandler::OnRequest(
    network::ResourceRequest* const request) {
  CHECK(request);

  const bool is_main_frame =
      request->resource_type ==
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  if (!is_main_frame) {
    // Don't notify content changes for subframes.
    return false;
  }

  content::NavigationEntry* const navigation_entry =
      GetPendingNavigationEntry();
  if (!navigation_entry || navigation_entry->IsRestored() ||
      IsInterstitialOrErrorPageNavigation(navigation_entry) ||
      !ui::PageTransitionIsNewNavigation(
          navigation_entry->GetTransitionType())) {
    // Don't notify content changes if the tab was restored, was a previously
    // committed navigation, or an error page was displayed.
    return false;
  }

  return true;
}

bool HtmlContentBodyHandler::ShouldProcess(
    const GURL& response_url,
    network::mojom::URLResponseHead* const response_head,
    bool* const defer) {
  CHECK(response_head);
  CHECK(defer);

  std::string mime_type;
  if (!response_head || !response_head->headers ||
      !response_head->headers->GetMimeType(&mime_type) ||
      !base::EqualsCaseInsensitiveASCII(mime_type, "text/html")) {
    return false;
  }

  if (HttpResponseHasErrorCode(response_head->headers.get())) {
    // Don't notify HTML content changes if an error page was displayed.
    return false;
  }

  *defer = false;

  return true;
}

void HtmlContentBodyHandler::OnComplete() {
  MaybeNotifyTabHtmlContentDidChange();
  html_ = std::string();
}

HtmlContentBodyHandler::Action HtmlContentBodyHandler::OnBodyUpdated(
    const std::string& body,
    const bool is_complete) {
  if (!is_complete) {
    return HtmlContentBodyHandler::Action::kContinue;
  }

  html_ = body;

  return HtmlContentBodyHandler::Action::kComplete;
}

bool HtmlContentBodyHandler::IsTransformer() const {
  return false;
}

void HtmlContentBodyHandler::HtmlContentBodyHandler::Transform(
    std::string /*body*/,
    base::OnceCallback<void(std::string)> /*on_complete*/) {
  NOTREACHED_NORETURN();
}

content::NavigationEntry* HtmlContentBodyHandler::GetPendingNavigationEntry() {
  content::WebContents* const web_contents = web_contents_getter_.Run();
  if (!web_contents) {
    return nullptr;
  }

  return web_contents->GetController().GetPendingEntry();
}

void HtmlContentBodyHandler::MaybeNotifyTabHtmlContentDidChange() {
  content::WebContents* const web_contents = web_contents_getter_.Run();
  if (!web_contents) {
    return;
  }

  Profile* const profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (!profile) {
    return;
  }

  const SessionID tab_id = GetTabIdFromWebContents(web_contents);
  if (!tab_id.is_valid()) {
    return;
  }

  const std::vector<GURL> redirect_chain =
      web_contents->GetController().GetVisibleEntry()->GetRedirectChain();
  // TODO: Investigate why redirect chain from NavigationEntry is empty for
  // brave://newtab
  if (redirect_chain.empty()) {
    return;
  }

  ads_service_->NotifyTabHtmlContentDidChange(tab_id.id(), redirect_chain,
                                              html_);
}

}  // namespace brave_ads

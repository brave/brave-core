// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/associated_url_content.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_errors.h"
#include "ui/base/page_transition_types.h"

namespace ai_chat {

AssociatedURLContent::AssociatedURLContent(
    GURL url,
    std::u16string title,
    content::BrowserContext* browser_context,
    base::OnceCallback<void(content::WebContents*)> attach_tab_helpers) {
  DVLOG(2) << __func__ << "Creating link content for: " << url.spec()
           << " title: " << title;

  set_uuid(base::Uuid::GenerateRandomV4().AsLowercaseString());
  set_url(std::move(url));
  SetTitle(std::move(title));

  // Create background WebContents optimized for headless loading
  content::WebContents::CreateParams params(browser_context);
  params.initially_hidden = true;
  params.preview_mode = true;
  web_contents_ = content::WebContents::Create(params);
  std::move(attach_tab_helpers).Run(web_contents_.get());

  // Start observing the WebContents
  content::WebContentsObserver::Observe(web_contents_.get());

  content_fetcher_ = std::make_unique<PageContentFetcher>(web_contents_.get());
}

AssociatedURLContent::~AssociatedURLContent() = default;

void AssociatedURLContent::GetContent(GetPageContentCallback callback) {
  // As we're just loading a link there's no point fetching it again if we have
  // content.
  // Note: If we change this in future we'll need to consider reloading the
  // page, as currently the DOMContentLoaded event won't fire again for
  // navigating to the same url.
  if (!cached_page_content().content.empty()) {
    std::move(callback).Run(cached_page_content());
    return;
  }

  if (!content_loaded_event_) {
    DVLOG(2) << __func__ << "Loading link content for: " << url().spec();

    content_loaded_event_ = std::make_unique<base::OneShotEvent>();

    // Note: We setup a 30 second timeout to avoid trying to load the page
    // forever.
    timeout_timer_.Start(FROM_HERE, base::Seconds(30),
                         base::BindOnce(&AssociatedURLContent::OnTimeout,
                                        weak_ptr_factory_.GetWeakPtr()));

    content::NavigationController::LoadURLParams load_params(url());
    load_params.transition_type = ui::PAGE_TRANSITION_LINK;
    web_contents_->GetController().LoadURLWithParams(load_params);
  }

  // Register callback with the OneShotEvent - it will be called when content is
  // loaded
  content_loaded_event_->Post(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<AssociatedURLContent> self,
                        GetPageContentCallback callback) {
                       if (!self) {
                         return;
                       }
                       std::move(callback).Run(self->cached_page_content());
                     },
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AssociatedURLContent::OnTimeout() {
  DVLOG(2) << __func__
           << "Background content loading timed out for URL: " << url().spec();

  // On timeout, try and fetch the page content anyway in case we managed to
  // partially load the page.
  SetTitle(web_contents_->GetTitle());
  content_fetcher_->FetchPageContent(
      /*invalidation_token=*/"",
      base::BindOnce(&AssociatedURLContent::OnContentExtractionComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AssociatedURLContent::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  if (navigation_handle->IsErrorPage()) {
    int net_error = navigation_handle->GetNetErrorCode();
    std::string error_description = net::ErrorToString(net_error);
    CompleteWithError("Navigation failed: " + error_description + " (" +
                      base::NumberToString(net_error) + ")");
  }
}

void AssociatedURLContent::DocumentOnLoadCompletedInPrimaryMainFrame() {
  DVLOG(2) << __func__ << "Page fully loaded for URL: " << url().spec();
  SetTitle(web_contents_->GetTitle());

  content_fetcher_->FetchPageContent(
      /*invalidation_token=*/"",
      base::BindOnce(&AssociatedURLContent::OnContentExtractionComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AssociatedURLContent::OnContentExtractionComplete(
    std::string content,
    bool is_video,
    std::string invalidation_token) {
  DVLOG(2) << __func__
           << "Content extraction completed for URL: " << url().spec()
           << ", content length: " << content.length()
           << ", is video: " << is_video;

  timeout_timer_.Stop();

  // Update our cached content with the loaded content
  set_cached_page_content(PageContent(std::move(content), is_video));

  // Notify pending callbacks
  if (content_loaded_event_) {
    content_loaded_event_->Signal();
    content_loaded_event_ = nullptr;
  }
}

void AssociatedURLContent::CompleteWithError(const std::string& error) {
  // Note: We don't actually do anything with the error, just log it in debug
  // mode.
  DVLOG(2) << __func__ << "Background content loading failed: " << error;

  timeout_timer_.Stop();

  // Clear cached content
  set_cached_page_content(PageContent());

  // Notify pending callbacks
  if (content_loaded_event_) {
    content_loaded_event_->Signal();
    content_loaded_event_ = nullptr;
  }
}

}  // namespace ai_chat

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/associated_url_content.h"

#include <utility>

#include "base/logging.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

namespace ai_chat {

AssociatedURLContent::AssociatedURLContent(
    GURL url,
    std::u16string title,
    std::unique_ptr<PrintPreviewExtractionDelegate>
        print_preview_extraction_delegate,
    std::unique_ptr<content::WebContents> web_contents)
    : AssociatedWebContentsContent(
          web_contents.get(),
          std::move(print_preview_extraction_delegate)),
      web_contents_(std::move(web_contents)) {
  DVLOG(2) << __func__ << "Creating link content for: " << url.spec()
           << " title: " << title;

  set_uuid(base::Uuid::GenerateRandomV4().AsLowercaseString());
  set_url(std::move(url));
  SetTitle(std::move(title));
}

AssociatedURLContent::~AssociatedURLContent() = default;

std::unique_ptr<AssociatedURLContent> AssociatedURLContent::Create(
    GURL url,
    std::u16string title,
    content::BrowserContext* browser_context,
    std::unique_ptr<PrintPreviewExtractionDelegate>
        print_preview_extraction_delegate,
    base::OnceCallback<void(content::WebContents*)> attach_tab_helpers) {
  // Create background WebContents optimized for headless loading
  content::WebContents::CreateParams params(browser_context);
  params.initially_hidden = true;
  params.preview_mode = true;
  auto web_contents = content::WebContents::Create(params);
  std::move(attach_tab_helpers).Run(web_contents.get());

  return std::make_unique<AssociatedURLContent>(
      url, title, std::move(print_preview_extraction_delegate),
      std::move(web_contents));
}

void AssociatedURLContent::GetPageContent(FetchPageContentCallback callback,
                                          std::string_view invalidation_token) {
  content::NavigationController::LoadURLParams load_params(url());
  load_params.transition_type = ui::PAGE_TRANSITION_LINK;
  web_contents_->GetController().LoadURLWithParams(load_params);

  AssociatedWebContentsContent::GetPageContent(std::move(callback),
                                               invalidation_token);
}

void AssociatedURLContent::OnNewPage(int64_t navigation_id) {
  // We don't ever treat URL content as a new page.
  // The only navigation is the initial navigation to the URL, and we don't want
  // that to be considered a new page.
}

}  // namespace ai_chat

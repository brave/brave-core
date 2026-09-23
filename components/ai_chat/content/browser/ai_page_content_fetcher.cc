// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/ai_page_content_fetcher.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/ai_chat/content/browser/page_content_blocks.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/content_extraction/ai_page_content.mojom.h"

namespace ai_chat {

AIPageContentFetcher::AIPageContentFetcher(content::WebContents* web_contents)
    : web_contents_(web_contents),
      page_content_fetcher_(
          std::make_unique<PageContentFetcher>(web_contents)) {}

AIPageContentFetcher::~AIPageContentFetcher() = default;

void AIPageContentFetcher::FetchPageContent(std::string_view invalidation_token,
                                            FetchPageContentCallback callback) {
  // Delegate to PageContentFetcher for URLs with custom extraction logic
  // (GitHub raw content, YouTube transcripts, Twitter screenshots, etc.)
  if (PageContentFetcher::HasCustomExtraction(
          web_contents_->GetLastCommittedURL())) {
    page_content_fetcher_->FetchPageContent(invalidation_token,
                                            std::move(callback));
    return;
  }

  CallGetAIPageContent(
      base::BindOnce(&AIPageContentFetcher::OnAIPageContentReceived,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIPageContentFetcher::CallGetAIPageContent(
    optimization_guide::OnAIPageContentDone callback) {
  // This is plain content extraction for AI Chat, so request the default
  // (non-actionable) page content. The actionable mode would make the renderer
  // compute geometry, hit-testing and scroller info that ConvertAnnotatedPage-
  // ContentToBlocks discards in `kContentOnly` mode anyway.
  optimization_guide::GetAIPageContent(
      web_contents_,
      optimization_guide::DefaultAIPageContentOptions(
          /*on_critical_path=*/false),
      std::move(callback));
}

void AIPageContentFetcher::OnAIPageContentReceived(
    FetchPageContentCallback callback,
    optimization_guide::AIPageContentResultOrError result) {
  if (!result.has_value()) {
    DVLOG(1) << "Error getting AI page content: " << result.error();
    std::move(callback).Run("", false, "");
    return;
  }

  // ConvertAnnotatedPageContentToBlocks always serializes everything into a
  // single TextContentBlock, so only index 0 is ever present. This is plain
  // content extraction (not agentic), so strip actionable element metadata and
  // SVG content that can't be reliably used here and only adds noise.
  auto content_blocks = ConvertAnnotatedPageContentToBlocks(
      result->proto, PageContentDetail::kContentOnly);
  if (content_blocks.empty() || !content_blocks[0]->is_text_content_block()) {
    std::move(callback).Run("", false, "");
    return;
  }

  std::move(callback).Run(
      std::move(content_blocks[0]->get_text_content_block()->text), false, "");
}

void AIPageContentFetcher::GetSearchSummarizerKey(
    mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback) {
  page_content_fetcher_->GetSearchSummarizerKey(std::move(callback));
}

void AIPageContentFetcher::GetOpenAIChatButtonNonce(
    mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback) {
  page_content_fetcher_->GetOpenAIChatButtonNonce(std::move(callback));
}

}  // namespace ai_chat

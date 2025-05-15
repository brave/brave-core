// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_chat/print_preview_extractor.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/browser/ui/ai_chat/print_preview_extractor_internal.h"
#include "brave/components/ai_chat/content/browser/pdf_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "printing/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace ai_chat {

PrintPreviewExtractor::PrintPreviewExtractor(content::WebContents* web_contents)
    : web_contents_(web_contents) {}

PrintPreviewExtractor::~PrintPreviewExtractor() = default;

void PrintPreviewExtractor::Extract(ExtractCallback callback) {
  // Overwrite any existing extraction in progress, cancelling the operation.
  // If AIChatTabHelper for this WebContents is asking for a new extraction
  // then it has navigated, or the previous extraction failed to report itself
  // somehow.
  extractor_ = std::make_unique<PrintPreviewExtractorInternal>(
      web_contents_,
      Profile::FromBrowserContext(web_contents_->GetBrowserContext()),
      IsPdf(web_contents_),
      PrintPreviewExtractorInternal::CallbackVariant(base::BindOnce(
          &PrintPreviewExtractor::OnComplete<ExtractCallback, std::string>,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
  extractor_->CreatePrintPreview();
}

void PrintPreviewExtractor::CapturePdf(CapturePdfCallback callback) {
  if (!IsPdf(web_contents_)) {
    std::move(callback).Run(base::unexpected("Not pdf content"));
    return;
  }
  // Overwrite any existing extraction in progress, cancelling the operation.
  extractor_ = std::make_unique<PrintPreviewExtractorInternal>(
      web_contents_,
      Profile::FromBrowserContext(web_contents_->GetBrowserContext()), true,
      PrintPreviewExtractorInternal::CallbackVariant(base::BindOnce(
          &PrintPreviewExtractor::OnComplete<CapturePdfCallback,
                                             std::vector<std::vector<uint8_t>>>,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
  extractor_->CreatePrintPreview();
}

template <typename CallbackType, typename ResultType>
void PrintPreviewExtractor::OnComplete(
    CallbackType callback,
    base::expected<ResultType, std::string> result) {
  extractor_.reset();
  std::move(callback).Run(std::move(result));
}

}  // namespace ai_chat

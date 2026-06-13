// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/print_preview_extraction_delegate_impl.h"

#include <utility>

namespace ai_chat {

PrintPreviewExtractionDelegateImpl::PrintPreviewExtractionDelegateImpl(
    content::WebContents* web_contents,
    std::unique_ptr<screenshot::PrintPreviewExtractor> extractor)
    : web_contents_(web_contents), extractor_(std::move(extractor)) {}

PrintPreviewExtractionDelegateImpl::~PrintPreviewExtractionDelegateImpl() =
    default;

void PrintPreviewExtractionDelegateImpl::CaptureImages(
    CaptureImagesCallback callback) {
  extractor_->CaptureImages(web_contents_, std::move(callback));
}

}  // namespace ai_chat

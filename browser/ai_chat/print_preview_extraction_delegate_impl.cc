// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/print_preview_extraction_delegate_impl.h"

#include <utility>

namespace ai_chat {

PrintPreviewExtractionDelegateImpl::PrintPreviewExtractionDelegateImpl(
    std::unique_ptr<screenshot::PrintPreviewExtractor> extractor)
    : extractor_(std::move(extractor)) {}

PrintPreviewExtractionDelegateImpl::~PrintPreviewExtractionDelegateImpl() =
    default;

void PrintPreviewExtractionDelegateImpl::CaptureImages(
    CaptureImagesCallback callback) {
  extractor_->CaptureImages(std::move(callback));
}

}  // namespace ai_chat

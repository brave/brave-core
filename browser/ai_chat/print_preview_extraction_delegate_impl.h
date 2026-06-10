// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_PRINT_PREVIEW_EXTRACTION_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_AI_CHAT_PRINT_PREVIEW_EXTRACTION_DELEGATE_IMPL_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/screenshot/print_preview_extractor.h"
#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "printing/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace ai_chat {

// Adapts the browser-layer PrintPreviewExtractor to the AI Chat
// PrintPreviewExtractionDelegate interface.
class PrintPreviewExtractionDelegateImpl
    : public AssociatedWebContentsContent::PrintPreviewExtractionDelegate {
 public:
  explicit PrintPreviewExtractionDelegateImpl(
      std::unique_ptr<screenshot::PrintPreviewExtractor> extractor);
  ~PrintPreviewExtractionDelegateImpl() override;

  PrintPreviewExtractionDelegateImpl(
      const PrintPreviewExtractionDelegateImpl&) = delete;
  PrintPreviewExtractionDelegateImpl& operator=(
      const PrintPreviewExtractionDelegateImpl&) = delete;

  // AssociatedWebContentsContent::PrintPreviewExtractionDelegate:
  void CaptureImages(CaptureImagesCallback callback) override;

 private:
  std::unique_ptr<screenshot::PrintPreviewExtractor> extractor_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_PRINT_PREVIEW_EXTRACTION_DELEGATE_IMPL_H_

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_PRINT_PREVIEW_EXTRACTOR_H_
#define BRAVE_BROWSER_AI_CHAT_PRINT_PREVIEW_EXTRACTOR_H_

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "printing/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace content {
class WebContents;
}  // namespace content

namespace ai_chat {

class PrintPreviewExtractor
    : public AIChatTabHelper::PrintPreviewExtractionDelegate {
 public:
  // Performs the print preview extraction. Used only for a single operation.
  class Extractor {
   public:
    using TextCallback =
        AIChatTabHelper::PrintPreviewExtractionDelegate::ExtractCallback;
    using ImageCallback =
        AIChatTabHelper::PrintPreviewExtractionDelegate::CapturePdfCallback;
    using CallbackVariant = std::variant<TextCallback, ImageCallback>;

    virtual ~Extractor() = default;
    virtual void CreatePrintPreview() = 0;
    virtual std::optional<int32_t> GetPrintPreviewUIIdForTesting() = 0;
  };

  using CreateExtractorCallback =
      base::RepeatingCallback<std::unique_ptr<Extractor>(
          content::WebContents* web_contents,
          bool is_pdf,
          Extractor::CallbackVariant&&)>;
  PrintPreviewExtractor(content::WebContents* web_contents,
                        CreateExtractorCallback callback);
  ~PrintPreviewExtractor() override;
  PrintPreviewExtractor(const PrintPreviewExtractor&) = delete;
  PrintPreviewExtractor& operator=(const PrintPreviewExtractor&) = delete;

  void Extract(ExtractCallback callback) override;

  void CapturePdf(CapturePdfCallback callback) override;

 private:
  friend class PrintPreviewExtractorTest;
  template <typename CallbackType, typename ResultType>
  void OnComplete(CallbackType callback,
                  base::expected<ResultType, std::string> result);

  CreateExtractorCallback create_extractor_callback_;
  std::unique_ptr<Extractor> extractor_;
  raw_ptr<content::WebContents> web_contents_;

  base::WeakPtrFactory<PrintPreviewExtractor> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_PRINT_PREVIEW_EXTRACTOR_H_

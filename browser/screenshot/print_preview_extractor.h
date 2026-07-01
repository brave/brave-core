// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SCREENSHOT_PRINT_PREVIEW_EXTRACTOR_H_
#define BRAVE_BROWSER_SCREENSHOT_PRINT_PREVIEW_EXTRACTOR_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "base/unguessable_token.h"
#include "content/public/browser/web_contents.h"
#include "printing/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace content {
class WebContents;
}  // namespace content

namespace screenshot {

class PrintPreviewExtractor {
 public:
  // Result is image data of pages or error
  using CaptureImagesCallback = base::OnceCallback<void(
      base::expected<std::vector<std::vector<uint8_t>>, std::string>)>;

  // Performs the print preview extraction. Used only for a single operation.
  class Extractor {
   public:
    virtual ~Extractor() = default;
    virtual void CreatePrintPreview() = 0;
    virtual base::UnguessableToken GetPrintPreviewUIIdForTesting() = 0;
  };

  using CreateExtractorCallback =
      base::RepeatingCallback<std::unique_ptr<Extractor>(
          content::WebContents* web_contents,
          bool is_pdf,
          CaptureImagesCallback&&)>;

  explicit PrintPreviewExtractor(CreateExtractorCallback callback);
  ~PrintPreviewExtractor();
  PrintPreviewExtractor(const PrintPreviewExtractor&) = delete;
  PrintPreviewExtractor& operator=(const PrintPreviewExtractor&) = delete;

  void CaptureImages(content::WebContents* web_contents,
                     CaptureImagesCallback callback);

 private:
  friend class PrintPreviewExtractorTest;
  void OnComplete(
      CaptureImagesCallback callback,
      base::expected<std::vector<std::vector<uint8_t>>, std::string> result);

  CreateExtractorCallback create_extractor_callback_;
  std::unique_ptr<Extractor> extractor_;

  base::WeakPtrFactory<PrintPreviewExtractor> weak_ptr_factory_{this};
};

}  // namespace screenshot

#endif  // BRAVE_BROWSER_SCREENSHOT_PRINT_PREVIEW_EXTRACTOR_H_

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/screenshot/print_preview_extractor.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/screenshot/content/pdf_utils.h"
#include "printing/buildflags/buildflags.h"

namespace screenshot {

PrintPreviewExtractor::PrintPreviewExtractor(content::WebContents* web_contents,
                                             CreateExtractorCallback callback)
    : create_extractor_callback_(std::move(callback)),
      web_contents_(web_contents) {}

PrintPreviewExtractor::~PrintPreviewExtractor() = default;

void PrintPreviewExtractor::CaptureImages(CaptureImagesCallback callback) {
  // Overwrite any existing extraction in progress, cancelling the operation.
  extractor_ = create_extractor_callback_.Run(
      web_contents_, screenshot::IsPdf(web_contents_),
      CaptureImagesCallback(base::BindOnce(&PrintPreviewExtractor::OnComplete,
                                           weak_ptr_factory_.GetWeakPtr(),
                                           std::move(callback))));
  extractor_->CreatePrintPreview();
}

void PrintPreviewExtractor::OnComplete(
    CaptureImagesCallback callback,
    base::expected<std::vector<std::vector<uint8_t>>, std::string> result) {
  extractor_.reset();
  std::move(callback).Run(std::move(result));
}

}  // namespace screenshot

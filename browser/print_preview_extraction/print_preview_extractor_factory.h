// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PRINT_PREVIEW_EXTRACTION_PRINT_PREVIEW_EXTRACTOR_FACTORY_H_
#define BRAVE_BROWSER_PRINT_PREVIEW_EXTRACTION_PRINT_PREVIEW_EXTRACTOR_FACTORY_H_

#include <memory>

#include "brave/browser/print_preview_extraction/print_preview_extractor.h"
#include "printing/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace content {
class WebContents;
}  // namespace content

namespace ai_chat {

// Constructs a PrintPreviewExtractor wired up with the default Extractor
// factory used in production (PrintPreviewExtractorInternal). The
// implementation lives in a file compiled into the chrome/browser umbrella so
// it can reach the print preview UI symbols without pulling chrome/browser/ui
// into a caller's dependency graph.
std::unique_ptr<PrintPreviewExtractor> CreateDefaultPrintPreviewExtractor(
    content::WebContents* web_contents);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_PRINT_PREVIEW_EXTRACTION_PRINT_PREVIEW_EXTRACTOR_FACTORY_H_

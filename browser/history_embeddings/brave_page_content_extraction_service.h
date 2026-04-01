// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PAGE_CONTENT_EXTRACTION_SERVICE_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PAGE_CONTENT_EXTRACTION_SERVICE_H_

#include "components/page_content_annotations/content/page_content_extraction_service.h"

namespace content {
class Page;
}

// Brave's PageContentExtractionService subclass that exposes content
// notification to be called by BravePageContentExtractionTabHelper.
// The upstream service's OnPageContentExtracted is protected; this
// subclass provides a public wrapper so our tab helper can feed
// extracted page content into the observer pipeline.
class BravePageContentExtractionService
    : public page_content_annotations::PageContentExtractionService {
 public:
  using PageContentExtractionService::PageContentExtractionService;

  // Public wrapper around protected OnPageContentExtracted.
  void NotifyPageContentExtracted(
      content::Page& page,
      scoped_refptr<
          const page_content_annotations::RefCountedAnnotatedPageContent> apc) {
    OnPageContentExtracted(page, std::move(apc), /*screenshot_data=*/{},
                           /*tab_id=*/std::nullopt);
  }
};

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PAGE_CONTENT_EXTRACTION_SERVICE_H_

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PAGE_CONTENT_EXTRACTION_TAB_HELPER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PAGE_CONTENT_EXTRACTION_TAB_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/mojom/content_extraction/ai_page_content.mojom.h"

class BravePageContentExtractionService;

// Extracts structured page content via Blink's AIPageContentAgent and feeds
// it to BravePageContentExtractionService, which distributes to observers
// (PageEmbeddingsService). This replaces the upstream
// PageContentAnnotationsWebContentsObserver which requires
// OptimizationGuideKeyedService that Brave doesn't use.
class BravePageContentExtractionTabHelper
    : public tabs::ContentsObservingTabFeature {
 public:
  BravePageContentExtractionTabHelper(
      tabs::TabInterface& tab,
      BravePageContentExtractionService* extraction_service);
  ~BravePageContentExtractionTabHelper() override;

  BravePageContentExtractionTabHelper(
      const BravePageContentExtractionTabHelper&) = delete;
  BravePageContentExtractionTabHelper& operator=(
      const BravePageContentExtractionTabHelper&) = delete;

 private:
  // tabs::ContentsObservingTabFeature:
  void DidStopLoading() override;

  void OnAIPageContentReceived(base::WeakPtr<content::Page> page,
                               blink::mojom::AIPageContentPtr content);

  mojo::Remote<blink::mojom::AIPageContentAgent> ai_page_content_agent_;

  // Not owned. Outlives this since it's a KeyedService for the profile.
  raw_ptr<BravePageContentExtractionService> extraction_service_;

  base::WeakPtrFactory<BravePageContentExtractionTabHelper> weak_ptr_factory_{
      this};
};

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PAGE_CONTENT_EXTRACTION_TAB_HELPER_H_

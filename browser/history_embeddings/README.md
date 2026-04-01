# browser/history_embeddings/

Brave's page content extraction pipeline for history embeddings
(semantic history search).

## Overview

Upstream Chromium's history embeddings relies on
`PageContentAnnotationsWebContentsObserver` to extract page content,
which requires `OptimizationGuideKeyedService` that Brave doesn't use.
This directory provides a Brave-specific replacement.

## Key Files

- **`brave_page_content_extraction_service.h`** — Subclass of
  `PageContentExtractionService` that exposes `NotifyPageContentExtracted()`
  so our tab helper can feed content into the upstream observer pipeline.

- **`brave_page_content_extraction_tab_helper.{h,cc}`** — Tab feature
  (`ContentsObservingTabFeature`) that extracts structured page content via
  Blink's `AIPageContentAgent` mojo on page load and passes it to the
  extraction service.

## Flow

```
BravePageContentExtractionTabHelper (DidStopLoading)
  → AIPageContentAgent mojo (renderer extracts DOM tree)
  → ConvertAIPageContentToProto (mojo → AnnotatedPageContent protobuf)
  → BravePageContentExtractionService::NotifyPageContentExtracted
    → PageEmbeddingsService (generates passage embeddings)
      → HistoryEmbeddingsService (stores with visit metadata)
```

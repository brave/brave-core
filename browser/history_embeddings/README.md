# browser/history_embeddings/

Brave's local embedder and content extraction pipeline for history
embeddings (semantic history search).

## Overview

Upstream Chromium's history embeddings uses OptimizationGuide for both
the embedding model and page content extraction. Brave replaces both:

- **Embedder**: `BraveEmbedder` uses the local EmbeddingGemma WASM model
  via `LocalAIService` instead of the OptimizationGuide model pipeline.
- **Content extraction**: `BravePageContentExtractionTabHelper` extracts
  page content via Blink's `AIPageContentAgent` mojo, bypassing the
  upstream `PageContentAnnotationsWebContentsObserver` which requires
  `OptimizationGuideKeyedService`.

## Key Files

- **`brave_embedder.{h,cc}`** — Implements `passage_embeddings::Embedder`.
  Processes passages sequentially, re-sorting the job queue by priority
  between each passage so high-priority search queries can preempt
  queued indexing work.

- **`brave_passage_embeddings_service_controller.{h,cc}`** — Singleton
  controller providing a shared `BraveEmbedder` backed by the guest
  profile's OTR `LocalAIService`. Uses an `EmbedderProxy` so
  `GetBraveEmbedder()` is always non-null even before the guest
  profile's async creation completes.

- **`brave_page_content_extraction_service.h`** — Subclass of
  `PageContentExtractionService` that exposes `NotifyPageContentExtracted()`
  so our tab helper can feed content into the upstream observer pipeline.

- **`brave_page_content_extraction_tab_helper.{h,cc}`** — Tab feature
  (`ContentsObservingTabFeature`) that extracts structured page content
  via Blink's `AIPageContentAgent` mojo on page load and passes it to
  the extraction service.

## Related Files

- **`components/history_embeddings/content/brave_history_embeddings_service.h`**
  — Template that overrides `OnPassageVisibilityCalculated` to synthesize
  passing visibility scores, since Brave doesn't use
  `PageContentAnnotationsService` for content visibility filtering.

- **`chromium_src/chrome/browser/history_embeddings/history_embeddings_service_factory.cc`**
  — Override to use `BravePassageEmbeddingsServiceController` and
  `BraveHistoryEmbeddingsService`.

- **`chromium_src/chrome/browser/page_content_annotations/`** — Factory
  overrides for `PageContentExtractionService` and `PageEmbeddingsService`.

## Flow

```
BravePageContentExtractionTabHelper (DidStopLoading)
  → AIPageContentAgent mojo (renderer extracts DOM tree)
  → ConvertAIPageContentToProto (mojo → AnnotatedPageContent protobuf)
  → BravePageContentExtractionService::NotifyPageContentExtracted
    → PageEmbeddingsService (upstream: chunks text into passages
      via kMaxWordsPerAggregatePassage, default 100 words)
      → BraveEmbedder (computes embeddings via LocalAIService WASM model)
        → HistoryEmbeddingsService (stores passages + embeddings with
          visit metadata for semantic search)
```

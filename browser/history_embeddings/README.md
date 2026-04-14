# browser/history_embeddings/

Brave's local embedder for history embeddings (semantic history search).

## Overview

Upstream Chromium's history embeddings uses `OptimizationGuide` to
deliver TFLite model files to `ChromePassageEmbeddingsServiceController`,
which creates a `SchedulingEmbedder` that communicates with
`PassageEmbeddingsService` (`services/passage_embeddings/`) running
in a separate utility process. Brave replaces this with a local WASM
model (EmbeddingGemma) running in a background WebContents via
`LocalAIService`, while reusing the upstream page content extraction
pipeline.

The upstream extraction pipeline is enabled by chromium_src overrides of
`OptimizationGuideKeyedServiceFactory`, `PageContentAnnotationsServiceFactory`,
and `PageContentExtractionServiceFactory` to check `kHistoryEmbeddings`
instead of the upstream feature flags.

## Key Files

- **`brave_embedder.{h,cc}`** — Implements `passage_embeddings::Embedder`
  (replacing upstream's `SchedulingEmbedder`). Processes passages
  sequentially, re-sorting the job queue by priority between each
  passage so high-priority search queries can preempt queued indexing
  work.

- **`brave_passage_embeddings_service_controller.{h,cc}`** — Singleton
  replacing `ChromePassageEmbeddingsServiceController`. Provides a
  single shared `BraveEmbedder` instance, created lazily on first
  `GetBraveEmbedder()` call.

## Related Files

- **`components/history_embeddings/content/brave_history_embeddings_service.h`**
  — Template wrapping `ChromeHistoryEmbeddingsService` that overrides
  `OnPassageVisibilityCalculated` to synthesize passing visibility
  scores, since Brave doesn't use `PageContentAnnotationsService` for
  content visibility filtering.

- **`chromium_src/chrome/browser/history_embeddings/history_embeddings_service_factory.cc`**
  — Override to use `BravePassageEmbeddingsServiceController` and
  `BraveHistoryEmbeddingsService`.

- **`chromium_src/chrome/browser/page_content_annotations/`** — Factory
  overrides for `PageContentAnnotationsService`, `PageContentExtractionService`,
  and `PageEmbeddingsService`.

- **`chromium_src/chrome/browser/optimization_guide/`** — Factory override
  to enable `OptimizationGuideKeyedService` when `kHistoryEmbeddings` is active.

## Flow

```
PageContentAnnotationsWebContentsObserver (upstream)
  → AnnotatedPageContentRequest (upstream extraction timing)
    → AIPageContentAgent mojo (renderer extracts DOM tree)
    → ConvertAIPageContentToProto (mojo → AnnotatedPageContent protobuf)
    → PageContentExtractionService::OnPageContentExtracted
      → PageEmbeddingsService (chunks text into passages)
        → BraveEmbedder (computes embeddings via LocalAIService WASM model)
          → HistoryEmbeddingsService (stores passages + embeddings)
```

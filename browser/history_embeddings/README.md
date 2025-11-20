# browser/history_embeddings/

Brave's local embedder for history embeddings (semantic history search).

## Overview

Upstream Chromium's history embeddings uses
[OptimizationGuide](https://source.chromium.org/chromium/chromium/src/+/main:components/optimization_guide/)
for the embedding model via
[`PassageEmbeddingsServiceController`](https://source.chromium.org/search?q=ChromePassageEmbeddingsServiceController).
Brave replaces the embedder with a local WASM model
([EmbeddingGemma](https://ai.google.dev/gemma/docs/embedding_gemma))
while using the upstream page content extraction pipeline.

The upstream extraction pipeline is enabled by chromium_src overrides of
`OptimizationGuideKeyedServiceFactory`, `PageContentAnnotationsServiceFactory`,
and `PageContentExtractionServiceFactory` to check `kHistoryEmbeddings`
instead of the upstream feature flags.

## Key Files

- **`brave_embedder.{h,cc}`** — Implements
  [`passage_embeddings::Embedder`](https://source.chromium.org/search?q=class%20Embedder%20passage_embeddings).
  Processes passages sequentially, re-sorting the job queue by priority
  between each passage so high-priority search queries can preempt
  queued indexing work.

- **`brave_passage_embeddings_service_controller.{h,cc}`** — Singleton
  replacing
  [`ChromePassageEmbeddingsServiceController`](https://source.chromium.org/search?q=ChromePassageEmbeddingsServiceController).
  Provides a single shared `BraveEmbedder` instance, created lazily on
  first `GetBraveEmbedder()` call.

## Related Files

- **`components/history_embeddings/content/brave_history_embeddings_service.h`**
  — Template wrapping
  [`ChromeHistoryEmbeddingsService`](https://source.chromium.org/search?q=ChromeHistoryEmbeddingsService)
  that overrides `OnPassageVisibilityCalculated` to synthesize passing
  visibility scores, since Brave doesn't use
  `PageContentAnnotationsService` for content visibility filtering.

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

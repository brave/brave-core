# browser/history_embeddings/

Brave's local embedder for history embeddings (semantic history search).

## Overview

Upstream Chromium's history embeddings uses `OptimizationGuide` to
deliver TFLite model files to `ChromePassageEmbeddingsServiceController`,
which binds its `service_remote_` to a `PassageEmbeddingsService`
(`services/passage_embeddings/`) running in a separate utility process.
The upstream-owned `SchedulingEmbedder` drives the job queue and calls
into that service.

Brave replaces only the transport: `BravePassageEmbeddingsServiceController`
launches an in-process `BravePassageEmbeddingsService` that implements
the same `passage_embeddings::mojom::PassageEmbeddingsService` mojom,
but routes inference through a background WebContents hosting a WASM
EmbeddingGemma worker on the guest OTR profile. The upstream
`SchedulingEmbedder` (priority re-sort, partial-progress resumption,
performance-scenario awareness) is unchanged.

The upstream extraction pipeline is enabled by chromium_src overrides of
`OptimizationGuideKeyedServiceFactory`, `PageContentAnnotationsServiceFactory`,
and `PageContentExtractionServiceFactory` to check `kHistoryEmbeddings`
instead of the upstream feature flags.

## Key Files

- **`brave_passage_embeddings_service.{h,cc}`** — In-process
  implementation of `passage_embeddings::mojom::PassageEmbeddingsService`.
  Owns the guest-OTR background WebContents, the
  `PassageEmbedderFactory` registration, and a
  `BraveBatchPassageEmbedder` that translates upstream's batch mojom to
  the renderer's one-passage-at-a-time interface. Also hosts the
  `WebContents*` → bind-callback registry used by `UntrustedLocalAIUI`.

- **`brave_passage_embeddings_service_controller.{h,cc}`** — Singleton
  replacing `ChromePassageEmbeddingsServiceController`. Overrides
  `MaybeLaunchService()`/`ResetServiceRemote()` to
  construct/destroy the in-process service. Fires the initial
  `EmbedderMetadataUpdated` notification in its constructor so the
  scheduler starts dispatching without the optimization_guide model
  info path.

## Related Files

- **`components/history_embeddings/content/brave_history_embeddings_service.h`**
  — Template wrapping `ChromeHistoryEmbeddingsService` that overrides
  `OnPassageVisibilityCalculated` to synthesize passing visibility
  scores, since Brave doesn't use `PageContentAnnotationsService` for
  content visibility filtering.

- **`chromium_src/chrome/browser/history_embeddings/history_embeddings_service_factory.cc`**
  — Override to use `BravePassageEmbeddingsServiceController` and
  `BraveHistoryEmbeddingsService`.

- **`chromium_src/components/passage_embeddings/core/passage_embeddings_service_controller.h`**
  — Chromium_src include shim that makes
  `EmbedderReady`/`GetEmbedderMetadata`/`GetEmbeddings` virtual and
  exposes `observer_list_` / `embedder_remote_` as protected so our
  subclass can override without an upstream patch.

- **`chromium_src/chrome/browser/page_content_annotations/`** — Factory
  overrides for `PageContentAnnotationsService`,
  `PageContentExtractionService`, and `PageEmbeddingsService`.

- **`chromium_src/chrome/browser/optimization_guide/`** — Factory
  override to enable `OptimizationGuideKeyedService` when
  `kHistoryEmbeddings` is active.

## Flow

```
PageContentAnnotationsWebContentsObserver (upstream)
  → AnnotatedPageContentRequest (upstream extraction timing)
    → AIPageContentAgent mojo (renderer extracts DOM tree)
    → ConvertAIPageContentToProto (mojo → AnnotatedPageContent protobuf)
    → PageContentExtractionService::OnPageContentExtracted
      → PageEmbeddingsService (chunks text into passages)
        → SchedulingEmbedder (upstream; queues, reorders by priority)
          → BravePassageEmbeddingsServiceController::GetEmbeddings
            → service_remote_->LoadModels (once) + GenerateEmbeddings
              → BravePassageEmbeddingsService::BraveBatchPassageEmbedder
                → WASM renderer (EmbeddingGemma)
                  → HistoryEmbeddingsService (stores passages + embeddings)
```

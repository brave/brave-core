# browser/history_embeddings/

Brave's local embedder for history embeddings (semantic history search).

## Overview

Upstream Chromium's history embeddings uses `OptimizationGuide` to deliver
TFLite model files to `ChromePassageEmbeddingsServiceController`, which binds
its `service_remote_` to a `PassageEmbeddingsService`
(`services/passage_embeddings/`) running in a sandboxed utility process. The
upstream-owned `SchedulingEmbedder` drives the job queue and calls into that
service.

Brave keeps that sandboxed-utility architecture but swaps the model and the
executor: `BravePassageEmbeddingsServiceController` subclasses
`PassageEmbeddingsServiceController` and launches the real utility via its
`LitertServiceLauncher`. Inside the utility, a chromium_src override of
`PassageEmbedderImpl::BuildExecutionTask` runs EmbeddingGemma natively through
LiteRT's `CompiledModel`
([`//brave/services/passage_embeddings:litert_model_runner`](../../services/passage_embeddings/))
instead of upstream's TFLite executor. The upstream `SchedulingEmbedder`
(priority re-sort, partial-progress resumption, performance-scenario awareness)
is unchanged.

The EmbeddingGemma model (`embeddinggemma-300M_seq512_mixed-precision.tflite`
plus its SentencePiece tokenizer) is delivered by the local AI component updater
and resolved from the component's `litert/` subdir; Brave does not use the
`optimization_guide` TFLite model.

The upstream extraction pipeline is enabled by chromium_src overrides of
`OptimizationGuideKeyedServiceFactory`, `PageContentAnnotationsServiceFactory`,
and `PageContentExtractionServiceFactory` to check `kHistoryEmbeddings` instead
of the upstream feature flags.

## Enabling / the brave://history toggle

Two prefs gate the feature, via the `IsHistoryEmbeddings*` overrides in
[`chromium_src/.../history_embeddings_utils.cc`](../../chromium_src/chrome/browser/history_embeddings/history_embeddings_utils.cc):

- **`kBraveLocalAIEnabled`** — local-state master switch (Brave Origin "Local
  AI"); gates `IsHistoryEmbeddingsFeatureEnabled()`.
- **`kBraveHistoryEmbeddingsEnabled`** — per-profile brave://history toggle;
  gates `IsHistoryEmbeddingsEnabledForProfile()`.

The embedder is built only when an embedding service feeds the controller. Both
such services (`PageEmbeddingsService`, `HistoryEmbeddingsService`) refuse to
build without `PassageEmbedderModelObserverFactory`, so
[`chromium_src/.../passage_embedder_model_observer_factory.cc`](../../chromium_src/chrome/browser/passage_embeddings/passage_embedder_model_observer_factory.cc)
gates that one shared dependency on `IsHistoryEmbeddingsEnabledForProfile()` —
toggle off ⇒ neither service is built ⇒ no embedder. (Its other upstream gates,
`kPassageEmbedder`/`kPermissionsAIv4`, are disabled in Brave.) Applied at
service creation, so changes take effect on restart.

## Model delivery + `EmbedderMetadataUpdated`

`SchedulingEmbedder` waits for `EmbedderMetadataUpdated` before it dispatches
any work, and the base controller opens the model files the paths point at.
Brave's model is delivered by the local AI component updater, so the controller
observes `LocalModelsUpdaterState`: when the EmbeddingGemma component is
installed, `OnLocalModelsReady` resolves
`embeddings_model_path_`/`sp_model_path_` from the component's `litert/` subdir
and fires `EmbedderMetadataUpdated`; `IsModelAvailable()` is true once those
paths are set. The metadata is otherwise static (`version=2`, `output_size=768`,
`threshold=0.45`); the version differs from the previous WASM embedder's so
`SqlDatabase` re-embeds stored history rather than mixing vector spaces.

The `chromium_src` include shim declares
`BravePassageEmbeddingsServiceController` as a `friend class` on the base so we
can set the model paths/metadata and reach `observer_list_` without touching the
upstream header (see
[`chromium_src/.../passage_embeddings_service_controller.h`](../../chromium_src/components/passage_embeddings/core/passage_embeddings_service_controller.h)).

## Key Files

- **`brave_passage_embeddings_service_controller.{h,cc}`** — Singleton subclass
  of `PassageEmbeddingsServiceController`. Provides `LitertServiceLauncher`,
  which launches the real sandboxed Passage Embeddings utility process. Observes
  `LocalModelsUpdaterState` and resolves the model paths from the component's
  `litert/` subdir in `OnLocalModelsReady`, publishes the LiteRT metadata, and
  swallows `optimization_guide` model updates (`MaybeUpdateModelInfo`) since
  Brave doesn't use that model. `GetEmbeddings` is the base implementation: the
  upstream launch + `LoadModels` flow opens the model files and drives the
  embedder in the utility.

- **`open_tab_search.{h,cc}`** — Standalone util, unrelated to the embedder
  above: it powers on-device "search my open tabs by content".
  `SearchOpenTabsByContent` snapshots a profile's open HTTP(S) tabs, resolves
  their URLs to URLIDs via `HistoryService`, then ranks them against a query
  with `HistoryEmbeddingsSearch::Search`. Shared by the tab_search WebUI page
  handler and the semantic tab search chat tool. Builds regardless of
  `enable_local_ai` since it only wraps upstream Chromium APIs.

## The LiteRT embedder

The native embedder lives in
[`//brave/services/passage_embeddings`](../../services/passage_embeddings/) and
runs inside the sandboxed utility, wired in by
[`chromium_src/services/passage_embeddings/passage_embedder_impl.cc`](../../chromium_src/services/passage_embeddings/passage_embedder_impl.cc):
its `BuildExecutionTask` override builds a `LitertModelRunner` from the loaded
`.tflite` and wraps it in upstream's `PassageEmbedderExecutor` interface.
`LitertModelRunner` runs EmbeddingGemma on LiteRT's CPU backend via
`litert::CompiledModel`. The browser opens the model files and sends them to the
utility through the standard `LoadModels` mojo call.

## Related Files

- **`components/history_embeddings/content/brave_history_embeddings_service.h`**
  — Template wrapping `ChromeHistoryEmbeddingsService` that overrides
  `OnPassageVisibilityCalculated` to synthesize passing visibility scores, since
  Brave doesn't use `PageContentAnnotationsService` for content visibility
  filtering.

- **`chromium_src/chrome/browser/passage_embeddings/passage_embedder_model_observer_factory.cc`**
  — Gates the per-profile `PassageEmbedderModelObserver` on the brave://history
  toggle so neither embedding service is built when it is off (see "Enabling /
  the brave://history toggle" above).

- **`chromium_src/chrome/browser/history_embeddings/history_embeddings_service_factory.cc`**
  — Override to use `BravePassageEmbeddingsServiceController` and
  `BraveHistoryEmbeddingsService`.

- **`chromium_src/components/passage_embeddings/core/passage_embeddings_service_controller.h`**
  — Chromium_src include shim. Adds `virtual` to
  `IsModelAvailable`/`GetEmbedderMetadata`/`MaybeUpdateModelInfo` via
  `#define`s, and declares
  `friend class BravePassageEmbeddingsServiceController` by macro-injecting it
  through the `EmbedderRunning` anchor (same idiom as
  `chromium_src/ui/android/view_android.h`).

- **`chromium_src/services/passage_embeddings/passage_embedder_impl.cc`** —
  Injects the LiteRT runner at the top of
  `PassageEmbedderImpl::BuildExecutionTask` (via a
  `BRAVE_PASSAGE_EMBEDDER_IMPL_BUILD_EXECUTION_TASK` macro), so the sandboxed
  utility runs EmbeddingGemma on LiteRT instead of upstream's TFLite executor.

- **`chromium_src/chrome/browser/page_content_annotations/`** — Factory
  overrides for `PageContentAnnotationsService`, `PageContentExtractionService`,
  and `PageEmbeddingsService`.

- **`chromium_src/chrome/browser/optimization_guide/`** — Factory override to
  enable `OptimizationGuideKeyedService` when `kHistoryEmbeddings` is active.

## Flow

```
PageContentAnnotationsWebContentsObserver (upstream)
  → AnnotatedPageContentRequest (upstream extraction timing)
    → AIPageContentAgent mojo (renderer extracts DOM tree)
    → ConvertAIPageContentToProto (mojo → AnnotatedPageContent protobuf)
    → PageContentExtractionService::OnPageContentExtracted
      → PageEmbeddingsService (chunks text into passages)
        → SchedulingEmbedder (upstream; queues, reorders by priority)
          → PassageEmbeddingsServiceController::GetEmbeddings (base)
            → !IsModelAvailable() → kModelUnavailable (SchedulingEmbedder
              retries on the next EmbedderMetadataUpdated)
            → LitertServiceLauncher launches the sandboxed utility
            → LoadModels (browser opens the .tflite + SentencePiece files and
              sends them to the utility)
              → PassageEmbedderImpl::BuildExecutionTask (chromium_src override)
                → LitertModelRunner (EmbeddingGemma on LiteRT's CompiledModel)
            → GenerateEmbeddings (per batch)
              → HistoryEmbeddingsService (stores passages + embeddings)
```

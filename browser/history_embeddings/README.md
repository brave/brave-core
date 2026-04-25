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
subclasses `PassageEmbeddingsServiceController` and swaps in an
in-process `BravePassageEmbeddingsService` that hosts a WASM
EmbeddingGemma worker inside a background WebContents on the guest
OTR profile. The upstream `SchedulingEmbedder` (priority re-sort,
partial-progress resumption, performance-scenario awareness) is
unchanged.

The upstream extraction pipeline is enabled by chromium_src overrides of
`OptimizationGuideKeyedServiceFactory`, `PageContentAnnotationsServiceFactory`,
and `PageContentExtractionServiceFactory` to check `kHistoryEmbeddings`
instead of the upstream feature flags.

## Why `BindPassageEmbedder` instead of mojo `LoadModels`

Upstream's `mojom::PassageEmbeddingsService::LoadModels` is shaped for
upstream's embedder: two `ReadOnlyFile` fields (tflite model +
sentencepiece tokenizer) and a `PassageEmbedderParams` of tflite-specific
knobs (thread counts per priority, cache size, GPU flag). EmbeddingGemma
needs five files (`weights`, `weights_dense1`, `weights_dense2`,
`tokenizer`, `config`) with different semantics, and none of the
`PassageEmbedderParams` tuning applies to our WASM renderer. There's no
clean way to map our inputs onto the upstream struct short of patching
the mojom.

The mojom also exists to cross a sandbox boundary — upstream runs its
`PassageEmbeddingsService` in a utility process. `BravePassageEmbeddingsService`
lives in the browser process, same process as the controller, so the
mojo pipe adds serialization cost without any isolation benefit.

The only useful piece `LoadModels` would carry for us is the
`PendingReceiver<PassageEmbedder>` that hooks the controller's
`embedder_remote_` up to the service. `BindPassageEmbedder(receiver,
callback)` carries exactly that and nothing more. The renderer's model
files are delivered separately through `PassageEmbedderFactory::Init` as
`local_ai::mojom::ModelFiles` (five `BigBuffer` fields), read from the
component-updater install directory surfaced by `LocalModelsUpdaterState`.

`BravePassageEmbeddingsService` still implements the upstream mojom for
completeness (its `LoadModels` override forwards to `BindPassageEmbedder`
after discarding the unused params), but the controller calls
`BindPassageEmbedder` directly. The base class's `service_remote_` is
left unbound; the `embedder_remote_` idle handler tears the whole
service down so the WASM renderer is freed.

## Why fire `EmbedderMetadataUpdated` from the constructor

`SchedulingEmbedder` waits for `EmbedderMetadataUpdated` before it
dispatches any work. Upstream fires it from `MaybeUpdateModelInfo()`
when optimization_guide delivers model files. Brave has no dynamic
model info — our metadata is static (`version=1`, `output_size=768`,
`threshold=0.45`) and always valid — so the controller fires the
notification once in its constructor. The chromium_src include shim
declares `BravePassageEmbeddingsServiceController` as a `friend class`
on the base so we can reach `observer_list_` and `embedder_remote_`
without touching the upstream header (see
[`chromium_src/.../passage_embeddings_service_controller.h`](../../chromium_src/components/passage_embeddings/core/passage_embeddings_service_controller.h)).

## Key Files

- **`brave_passage_embeddings_service.{h,cc}`** — In-process
  implementation of `passage_embeddings::mojom::PassageEmbeddingsService`
  (and `local_ai::mojom::LocalAIService` for renderer binding).
  Exposes a direct `BindPassageEmbedder(...)` entry point used by the
  controller. Owns the guest-OTR background WebContents, the
  `PassageEmbedderFactory` registration, and a
  `BraveBatchPassageEmbedder` that translates upstream's batch mojom
  to the renderer's one-passage-at-a-time interface. Also hosts the
  static `WebContents*` → bind-callback registry used by
  `UntrustedLocalAIUI::BindInterface`.

- **`brave_passage_embeddings_service_controller.{h,cc}`** — Singleton
  subclass of `PassageEmbeddingsServiceController`. Overrides
  `MaybeLaunchService()`/`ResetServiceRemote()` to construct/destroy
  the in-process service, `EmbedderReady()`/`GetEmbedderMetadata()`
  to report static metadata, and `GetEmbeddings()` to call
  `service_->BindPassageEmbedder()` directly and route batches to the
  bound `embedder_remote_`. Fires the initial
  `EmbedderMetadataUpdated` notification in its constructor.

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
  — Chromium_src include shim. Adds `virtual` to
  `EmbedderReady`/`GetEmbedderMetadata`/`GetEmbeddings` via
  `#define`s, and declares
  `friend class BravePassageEmbeddingsServiceController` by
  macro-injecting it through the `EmbedderRunning` anchor (same
  idiom as `chromium_src/ui/android/view_android.h`).

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
            → service_->BindPassageEmbedder (first time, in-process)
              → PassageEmbedderFactory::Init (loads WASM model)
                → BraveBatchPassageEmbedder bound to embedder_remote_
            → embedder_remote_->GenerateEmbeddings (subsequent batches)
              → WASM renderer (EmbeddingGemma, one passage at a time)
                → HistoryEmbeddingsService (stores passages + embeddings)
```

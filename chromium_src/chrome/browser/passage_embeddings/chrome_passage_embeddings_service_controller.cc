/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/passage_embeddings/chrome_passage_embeddings_service_controller.h"

#include "base/process/process.h"
#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

// This file REPLACES upstream's
// chrome/browser/passage_embeddings/
//     chrome_passage_embeddings_service_controller.cc via Brave's
// chromium_src override mechanism. Chrome's implementation launches a
// sandboxed utility process and drives CpuHistogramLogger; Brave's
// service runs in-process inside the browser process, so those code
// paths have no Brave equivalent. Instead of pulling the full upstream
// file in and redefining Get(), we re-declare the class members as
// trivial stubs and provide our own Get() that returns the Brave
// singleton.
//
// BravePassageEmbeddingsServiceController publicly inherits from
// ChromePassageEmbeddingsServiceController, so Get() upcasts to the
// declared return type. Every upstream caller —
// HistoryEmbeddingsServiceFactory, PageEmbeddingsServiceFactory,
// PassageEmbedderModelObserver — resolves to the Brave instance
// without per-factory chromium_src swaps.

namespace passage_embeddings {

// static
ChromePassageEmbeddingsServiceController*
ChromePassageEmbeddingsServiceController::Get() {
  return BravePassageEmbeddingsServiceController::Get();
}

ChromePassageEmbeddingsServiceController::
    ChromePassageEmbeddingsServiceController() = default;

ChromePassageEmbeddingsServiceController::
    ~ChromePassageEmbeddingsServiceController() = default;

// Brave's subclass overrides both MaybeLaunchService and
// ResetServiceRemote; these base-class stubs exist only so the vtable
// links. They should never be reached at runtime.
void ChromePassageEmbeddingsServiceController::MaybeLaunchService() {}

void ChromePassageEmbeddingsServiceController::ResetServiceRemote() {}

void ChromePassageEmbeddingsServiceController::OnServiceLaunched(
    const base::Process& process) {}

void ChromePassageEmbeddingsServiceController::InitializeCpuLogger() {}

}  // namespace passage_embeddings

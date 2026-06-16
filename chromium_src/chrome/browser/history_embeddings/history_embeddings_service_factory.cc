/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"

#include "brave/components/local_ai/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/browser/history_embeddings/brave_history_embeddings_service.h"

// Swap Chrome's service for Brave's concrete subclass. Brave overrides
// OnPassageVisibilityCalculated to synthesize a passing visibility score
// since Brave doesn't use PageContentAnnotationsService.
#define ChromeHistoryEmbeddingsService BraveHistoryEmbeddingsService
#endif

#include <chrome/browser/history_embeddings/history_embeddings_service_factory.cc>

#if BUILDFLAG(ENABLE_LOCAL_AI)
#undef ChromeHistoryEmbeddingsService
#endif

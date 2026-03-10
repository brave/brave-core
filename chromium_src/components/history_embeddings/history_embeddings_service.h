/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_SERVICE_H_

// Make OnPassageVisibilityCalculated virtual and protected so
// BraveHistoryEmbeddingsService can override it to skip the
// scored_url_rows.clear() when annotations are empty (Brave doesn't
// use PageContentAnnotationsService).
#define OnPassageVisibilityCalculated \
  NotUsed() {}                        \
                                      \
 protected:                           \
  virtual void OnPassageVisibilityCalculated

#include <components/history_embeddings/history_embeddings_service.h>  // IWYU pragma: export

#undef OnPassageVisibilityCalculated

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_SERVICE_H_

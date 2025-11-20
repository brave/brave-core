/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PASSAGE_EMBEDDINGS_CORE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PASSAGE_EMBEDDINGS_CORE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

// Make EmbedderReady, GetEmbedderMetadata, and GetEmbeddings virtual so Brave
// can override them
#define EmbedderReady virtual EmbedderReady
#define GetEmbedderMetadata virtual GetEmbedderMetadata
#define GetEmbeddings virtual GetEmbeddings

#include <components/passage_embeddings/core/passage_embeddings_service_controller.h>  // IWYU pragma: export

#undef GetEmbeddings
#undef GetEmbedderMetadata
#undef EmbedderReady

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PASSAGE_EMBEDDINGS_CORE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Include the features header first so the declaration is visible,
// then redefine kHistoryEmbeddings to our flag for the test body.
#include "brave/components/local_ai/core/features.h"
#include "components/history_embeddings/history_embeddings_features.h"

// Redirect the upstream feature reference so the test enables
// kBraveHistoryEmbeddings instead, which is what our factory checks.
#define kHistoryEmbeddings kBraveHistoryEmbeddings_Unused
namespace history_embeddings {
inline constexpr auto& kBraveHistoryEmbeddings_Unused =
    local_ai::features::kBraveHistoryEmbeddings;
}  // namespace history_embeddings

#include <chrome/browser/ui/webui/cr_components/history_embeddings/history_embeddings_handler_unittest.cc>

#undef kHistoryEmbeddings

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_selected_keyword_view_util.h"

#include "brave/components/local_ai/buildflags/buildflags.h"
#include "brave/components/vector_icons/vector_icons.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/browser/history_embeddings/history_embeddings_utils.h"
#endif

const gfx::VectorIcon& GetAskBraveSearchStarterPackIcon() {
  return kLeoMessageBubbleAskIcon;
}

bool BraveIsHistoryEmbeddingsEnabledForProfile(Profile* profile) {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  return brave_history_embeddings::IsHistoryEmbeddingsEnabledForProfile(
      profile);
#else
  return false;
#endif
}

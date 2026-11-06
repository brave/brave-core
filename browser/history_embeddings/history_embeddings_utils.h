// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_UTILS_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_UTILS_H_

class Profile;

namespace brave_history_embeddings {

// Whether semantic history search can run at all: the Local AI master switch
// is on and `kHistoryEmbeddings` is enabled. Does not check the per-profile
// setting.
bool IsHistoryEmbeddingsFeatureEnabled();

// Whether semantic history search is enabled for `profile`: the feature is
// enabled and the profile's Semantic history search setting is on. This reads
// the live setting; the services are built from it once at profile setup.
bool IsHistoryEmbeddingsEnabledForProfile(Profile* profile);

}  // namespace brave_history_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_UTILS_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/features.h"

namespace local_ai::features {

// Master switch for Brave's semantic history search. When enabled, activates
// chromium's history embeddings infrastructure, triggers local AI model
// downloads via the component updater, and registers the WASM-based
// embedding WebUI.
BASE_FEATURE(kBraveHistoryEmbeddings, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace local_ai::features

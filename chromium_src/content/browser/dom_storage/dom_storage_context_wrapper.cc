/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Brave uses opaque origins to access ephemeral localStorage areas. This block
// allows these origins to be treated as valid ones.
#define BRAVE_DOM_STORAGE_CONTEXT_WRAPPER_IS_REQUEST_VALID                 \
  if (host_storage_key_did_not_match && storage_key.origin().opaque() &&   \
      host->GetStorageKey().origin().GetTupleOrPrecursorTupleIfOpaque() == \
          storage_key.origin().GetTupleOrPrecursorTupleIfOpaque()) {       \
    host_storage_key_did_not_match = false;                                \
  }

#include "src/content/browser/dom_storage/dom_storage_context_wrapper.cc"

#undef BRAVE_DOM_STORAGE_CONTEXT_WRAPPER_IS_REQUEST_VALID

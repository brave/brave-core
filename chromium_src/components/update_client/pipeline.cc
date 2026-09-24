/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Component updater needs to supply `size` attribute with the `download`
// operations. Until our go-updater gets that attribute in we can get bypass
// the upstream check.
#define BRAVE_MAKE_OPERATIONS                         \
  if (operation.urls.empty() || operation.size < 0 || \
      operation.sha256_out.empty())

#include <components/update_client/pipeline.cc>
#undef BRAVE_MAKE_OPERATIONS

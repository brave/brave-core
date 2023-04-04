/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/embeddings_database_util.h"

#include "base/bind.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/embeddings_database_table.h"

namespace ads::database {

void DeleteEmbeddings() {
  const table::Embeddings database_table;
  database_table.Delete(base::BindOnce([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to delete embeddings");
      return;
    }

    BLOG(3, "Successfully deleted embeddings");
  }));
}

}  // namespace ads::database

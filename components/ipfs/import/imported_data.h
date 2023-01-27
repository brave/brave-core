/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IMPORT_IMPORTED_DATA_H_
#define BRAVE_COMPONENTS_IPFS_IMPORT_IMPORTED_DATA_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "url/gurl.h"

namespace ipfs {

enum ImportState {
  IPFS_IMPORT_SUCCESS,
  IPFS_IMPORT_ERROR_REQUEST_EMPTY,
  IPFS_IMPORT_ERROR_ADD_FAILED,
  IPFS_IMPORT_ERROR_MKDIR_FAILED,
  IPFS_IMPORT_ERROR_MOVE_FAILED,
  IPFS_IMPORT_ERROR_PUBLISH_FAILED,
};

struct ImportedData {
  ImportedData();
  ~ImportedData();

  std::string hash;
  std::string published_key;
  int64_t size = -1;
  std::string directory;
  std::string filename;
  ImportState state;
};

using ImportCompletedCallback =
    base::OnceCallback<void(const ipfs::ImportedData&)>;

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IMPORT_IMPORTED_DATA_H_

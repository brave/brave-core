// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/download/download_item_model.h"

#include "src/chrome/browser/download/download_item_model.cc"

void DownloadItemModel::DeleteLocalFile() {
  download_->DeleteFile(base::DoNothing());
}

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/extensions/api/image_writer_private/removable_storage_provider.h"
#include "content/public/browser/browser_thread.h"

namespace extensions {
// static
scoped_refptr<StorageDeviceList>
RemovableStorageProvider::PopulateDeviceList() {
  return nullptr;
}

}  // namespace extensions

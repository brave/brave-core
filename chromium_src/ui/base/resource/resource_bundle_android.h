// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_BASE_RESOURCE_RESOURCE_BUNDLE_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_UI_BASE_RESOURCE_RESOURCE_BUNDLE_ANDROID_H_

#include <ui/base/resource/resource_bundle_android.h>  // IWYU pragma: export

#include "base/files/memory_mapped_file.h"

namespace ui {

COMPONENT_EXPORT(UI_BASE)
void BraveLoadMainAndroidPackFile(const char* path_within_apk,
                                  const base::FilePath& disk_file_path);

// Returns the file descriptor and mapped region of brave_resources.pak opened
// by the browser process, so it can be shared with child processes that have no
// JVM to open the APK asset themselves. Returns -1 if the pak was not loaded.
COMPONENT_EXPORT(UI_BASE)
int GetBraveResourcesPackFd(base::MemoryMappedFile::Region* out_region);

}  // namespace ui

#endif  // BRAVE_CHROMIUM_SRC_UI_BASE_RESOURCE_RESOURCE_BUNDLE_ANDROID_H_

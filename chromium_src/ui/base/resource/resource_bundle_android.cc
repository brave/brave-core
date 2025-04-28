// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/ui/base/resource/resource_bundle_android.cc"

namespace ui {

namespace {

int g_brave_resources_pack_fd = -1;
int g_dev_ui_resources_pack_fd = -1;

base::MemoryMappedFile::Region g_brave_resources_pack_region;
base::MemoryMappedFile::Region g_dev_ui_resources_pack_region;
}

void BraveLoadAdditionalAndroidPackFiles() {
  base::FilePath disk_file_path;

  // brave_100_percent.pak is excluded now from the Android build because
  // its resources are not used
  if (LoadFromApkOrFile("assets/brave_resources.pak", &disk_file_path,
                        &g_brave_resources_pack_fd,
                        &g_brave_resources_pack_region)) {
    ResourceBundle::GetSharedInstance().AddDataPackFromFileRegion(
        base::File(g_brave_resources_pack_fd), g_brave_resources_pack_region,
        ui::kScaleFactorNone);
  }

  // dev_ui_resources.pak is required only for the case when the universal
  // apk was generated from aab, with --android_aab_to_apk build key.
  // For the aab bundle dev_ui_resources.pak is placed into a separate module,
  // dev_ui-master.apk and with --android_aab_to_apk it goes into
  // Bravearm64Universal.apk as-is, so we need to force load. The regular apk
  // has dev-ui resources merged into resources.pak.
  if (LoadFromApkOrFile("assets/dev_ui_resources.pak", &disk_file_path,
                        &g_dev_ui_resources_pack_fd,
                        &g_dev_ui_resources_pack_region)) {
    ResourceBundle::GetSharedInstance().AddDataPackFromFileRegion(
        base::File(g_dev_ui_resources_pack_fd), g_dev_ui_resources_pack_region,
        ui::kScaleFactorNone);
  }
}

}  // namespace ui

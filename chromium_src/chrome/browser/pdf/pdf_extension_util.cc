/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/pdf/pdf_extension_util.h"

#define GetManifest GetManifest_ChromiumImpl

#include "src/chrome/browser/pdf/pdf_extension_util.cc"

#undef GetManifest

namespace pdf_extension_util {

std::string GetManifest() {
  std::string manifest_contents =
      pdf_extension_util::GetManifest_ChromiumImpl();
  base::ReplaceFirstSubstringAfterOffset(
      &manifest_contents, 0, "Chromium PDF Viewer", "Chrome PDF Viewer");
  return manifest_contents;
}

}  // namespace pdf_extension_util

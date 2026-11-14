// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PROFILES_CUSTOM_PROFILE_IMAGE_IMAGE_FILE_H_
#define BRAVE_BROWSER_PROFILES_CUSTOM_PROFILE_IMAGE_IMAGE_FILE_H_

#include <cstdint>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "brave/ui/webui/custom_profile_image/buildflags/buildflags.h"
#include "third_party/skia/include/core/SkBitmap.h"

static_assert(BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE));

namespace custom_profile_image {

// Files in the profile directory: the saved image and its staging copy.
inline constexpr char kImageFileName[] = "Brave Profile Image.png";
inline constexpr char kPendingImageFileName[] = "Brave Profile Image.pending";
// The largest stored width and height, in pixels.
inline constexpr int kMaxImageSide = 512;

namespace internal {
// Whether `bytes` start with a PNG header for a square image of at most
// kMaxImageSide px.
bool HasNormalizedHeader(base::span<const uint8_t> bytes);
}  // namespace internal

// Used only on the profile's sequenced file runner. Input is a normalized
// bitmap, never renderer-provided encoded data. No operation creates a profile
// directory. A failed replacement leaves the previous PNG intact.
class ImageFile {
 public:
  explicit ImageFile(const base::FilePath& profile_path);
  ~ImageFile();

  ImageFile(const ImageFile&) = delete;
  ImageFile& operator=(const ImageFile&) = delete;

  SkBitmap Read();
  bool Replace(const SkBitmap& bitmap);
  bool Remove();

 private:
  const base::FilePath path_;
  const base::FilePath pending_path_;
};

}  // namespace custom_profile_image

#endif  // BRAVE_BROWSER_PROFILES_CUSTOM_PROFILE_IMAGE_IMAGE_FILE_H_

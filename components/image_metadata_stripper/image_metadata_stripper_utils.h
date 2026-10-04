/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_UTILS_H_
#define BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_UTILS_H_

#include <stddef.h>

#include <optional>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"

namespace image_metadata_stripper {

// Prefix passed to CreateUniqueTempDir for the directory holding the copies a
// `StrippedImageCopier` makes.
inline constexpr base::FilePath::CharType kStripTempDirPrefix[] =
    FILE_PATH_LITERAL("brave_image_strip");

// Recursively deletes |temp_root_dir|, a directory previously taken from a
// `StrippedImageCopier`, along with every copy it holds. No-ops when
// |temp_root_dir| is not one of ours, so that a caller passing a path it
// collected elsewhere cannot delete an unrelated directory.
bool DeleteStrippedImageCopies(const base::FilePath& temp_root_dir);

// Makes metadata-free copies of images, for the flows that have to hand a file
// to a web page without modifying the file the user picked.
//
// Each copy goes in a subdirectory of its own under one temporary root
// directory, and keeps the basename of its original, so whatever reads the copy
// still sees the name the user chose. A copy that could not be written or
// stripped is discarded, leaving the caller to use the original.
//
// Every method blocks, so this must be used on a thread that allows blocking.
// The root directory is deleted when this object goes out of scope, unless
// `TakeTempRootDir` is called; a caller handing the copies to a renderer has to
// take it, since the copies must outlive the strip itself.
//
//   StrippedImageCopier copier(StrippingClient::kFileSelect);
//   for (Entry& entry : entries) {
//     if (auto copy = copier.MaybeCreateStrippedCopy(entry.path)) {
//       entry.path = *copy;
//     }
//   }
//   if (copier.has_copies()) {
//     queue_for_deletion(copier.TakeTempRootDir());
//   }
class StrippedImageCopier {
 public:
  explicit StrippedImageCopier(StrippingClient client);
  ~StrippedImageCopier();

  StrippedImageCopier(const StrippedImageCopier&) = delete;
  StrippedImageCopier& operator=(const StrippedImageCopier&) = delete;

  // Returns the path of a metadata-free copy of |src|, or nullopt when |src|
  // carries no metadata to strip, or when the copy could not be made. In every
  // nullopt case the caller should keep using |src|.
  std::optional<base::FilePath> MaybeCreateStrippedCopy(
      const base::FilePath& src);

  // Whether any call to `MaybeCreateStrippedCopy` produced a copy, and so
  // whether there is a temporary directory worth taking.
  bool has_copies() const { return has_copies_; }

  // Hands the temporary root directory over to the caller, which becomes
  // responsible for deleting it via `DeleteStrippedImageCopies` once the copies
  // are no longer reachable.
  base::FilePath TakeTempRootDir();

 private:
  // Creates the temporary root directory on first use, so that a flow with
  // nothing to strip never touches the disk.
  bool EnsureTempRootDir();

  const StrippingClient client_;
  base::ScopedTempDir temp_root_;
  // Names the subdirectory of the next copy, keeping copies of identically
  // named originals apart.
  size_t next_sub_dir_index_ = 0;
  bool has_copies_ = false;
};

}  // namespace image_metadata_stripper

#endif  // BRAVE_COMPONENTS_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_UTILS_H_

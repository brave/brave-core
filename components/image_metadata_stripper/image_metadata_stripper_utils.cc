/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/image_metadata_stripper/image_metadata_stripper_utils.h"

#include <optional>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"

namespace image_metadata_stripper {

bool DeleteStrippedImageCopies(const base::FilePath& temp_root_dir) {
  // Gaurd from randomly deleting unrelated folders.
  if (temp_root_dir.BaseName().value().find(kStripTempDirPrefix) ==
      base::FilePath::StringType::npos) {
    LOG(ERROR) << "Tried to delete a non owning directory. dir: "
               << temp_root_dir;
    return false;
  }

  return base::DeletePathRecursively(temp_root_dir);
}

StrippedImageCopier::StrippedImageCopier(StrippingClient client)
    : client_(client) {}

StrippedImageCopier::~StrippedImageCopier() = default;

std::optional<base::FilePath> StrippedImageCopier::MaybeCreateStrippedCopy(
    const base::FilePath& src) {
  const base::FilePath basename = src.BaseName();
  // "" or "../" cannot name the copy, and the latter could escape the isolation
  // the temporary root directory provides.
  if (basename.empty() || basename.ReferencesParent()) {
    return std::nullopt;
  }

  if (!IsSupportedImagePath(src) || !ContainsMetadataToStrip(src)) {
    return std::nullopt;
  }

  if (!EnsureTempRootDir()) {
    return std::nullopt;
  }

  // Give the copy a subdirectory of its own so that it can keep the basename of
  // |src| without colliding with the other copies.
  base::ScopedTempDir sub_dir;
  if (!sub_dir.Set(temp_root_.GetPath().AppendASCII(
          base::NumberToString(next_sub_dir_index_++)))) {
    LOG(ERROR) << "Image strip skipped; temp subdir could not be created: "
               << src;
    return std::nullopt;
  }

  const base::FilePath copy = sub_dir.GetPath().Append(basename);
  if (!base::CopyFile(src, copy)) {
    DVLOG(1) << "Image strip skipped; failed to copy the image file to a "
                "temporary file.";
    return std::nullopt;
  }

  if (!RemoveIptcMetadata(client_, copy)) {
    DVLOG(1) << "No stripping occured; keeping original: " << src;
    // ScopedTempDir deletes the subdir, so the failed copy leaves nothing
    // behind.
    return std::nullopt;
  }

  // Keep the copy under the root until the caller is done with it.
  sub_dir.Take();
  has_copies_ = true;
  return copy;
}

base::FilePath StrippedImageCopier::TakeTempRootDir() {
  CHECK(has_copies_);
  return temp_root_.Take();
}

bool StrippedImageCopier::EnsureTempRootDir() {
  if (temp_root_.IsValid()) {
    return true;
  }

  if (!temp_root_.CreateUniqueTempDir(kStripTempDirPrefix)) {
    LOG(ERROR) << "Image strip skipped; temp directory could not be created.";
    return false;
  }

  return true;
}

}  // namespace image_metadata_stripper

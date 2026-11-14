// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/profiles/custom_profile_image/image_file.h"

#include <string>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/numerics/byte_conversions.h"
#include "ui/gfx/codec/png_codec.h"

namespace custom_profile_image {

namespace {
// An uncompressed RGBA image plus PNG framing fits within this limit.
constexpr size_t kMaxStoredBytes = 2 * 1024 * 1024;

bool IsNormalized(const SkBitmap& bitmap) {
  return !bitmap.drawsNothing() && bitmap.width() == bitmap.height() &&
         bitmap.width() <= kMaxImageSide;
}
}  // namespace

namespace internal {
// Checks the IHDR dimensions so a damaged or replaced file cannot make the
// browser allocate a large bitmap before IsNormalized() rejects it.
bool HasNormalizedHeader(base::span<const uint8_t> bytes) {
  static constexpr uint8_t kSignature[] = {0x89, 'P',  'N',  'G',
                                           '\r', '\n', 0x1a, '\n'};
  static constexpr uint8_t kHeaderChunk[] = {0, 0, 0, 13, 'I', 'H', 'D', 'R'};
  if (bytes.size() < 24 || bytes.first<8>() != base::span(kSignature) ||
      bytes.subspan<8, 8>() != base::span(kHeaderChunk)) {
    return false;
  }
  const uint32_t width = base::U32FromBigEndian(bytes.subspan<16, 4>());
  const uint32_t height = base::U32FromBigEndian(bytes.subspan<20, 4>());
  return width > 0 && width == height && width <= kMaxImageSide;
}
}  // namespace internal

ImageFile::ImageFile(const base::FilePath& profile_path)
    : path_(profile_path.AppendASCII(kImageFileName)),
      pending_path_(profile_path.AppendASCII(kPendingImageFileName)) {}

ImageFile::~ImageFile() = default;

SkBitmap ImageFile::Read() {
  // A pending file was never committed. Do not promote it after a crash.
  base::DeleteFile(pending_path_);
  std::string bytes;
  if (!base::ReadFileToStringWithMaxSize(path_, &bytes, kMaxStoredBytes) ||
      !internal::HasNormalizedHeader(base::as_byte_span(bytes))) {
    return {};
  }
  SkBitmap bitmap = gfx::PNGCodec::Decode(base::as_byte_span(bytes));
  return IsNormalized(bitmap) ? bitmap : SkBitmap();
}

bool ImageFile::Replace(const SkBitmap& bitmap) {
  if (!IsNormalized(bitmap)) {
    return false;
  }
  auto bytes = gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false);
  if (!bytes || bytes->size() > kMaxStoredBytes) {
    return false;
  }

  // Flush the new file before publishing it. The fixed staging name is safe
  // because every operation for this profile uses the same sequence.
  base::File pending(pending_path_,
                     base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!pending.IsValid()) {
    return false;
  }
  const bool written =
      pending.WriteAtCurrentPosAndCheck(*bytes) && pending.Flush();
  pending.Close();
  if (!written || !base::ReplaceFile(pending_path_, path_, nullptr)) {
    base::DeleteFile(pending_path_);
    return false;
  }
  return true;
}

bool ImageFile::Remove() {
  // DeleteFile treats an absent file as success and does not remove a nonempty
  // directory. Keep failures visible to the caller before it changes metadata.
  return base::DeleteFile(pending_path_) && base::DeleteFile(path_);
}

}  // namespace custom_profile_image

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_tar_decompressor.h"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/numerics/safe_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_checksum_calculator.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_tar_utils.h"
#include "third_party/zlib/google/compression_utils.h"

namespace brave_wallet {

SnapTarballExtractOutcome SnapInstallerTarDecompressor::ExtractTarballToDir(
    const base::FilePath& tarball_path) {
  // Blocking file I/O on the calling thread; the caller must allow it. The
  // guard cleans the dir up on any failure path below; ownership is released
  // to the caller on success.
  base::ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDir(FILE_PATH_LITERAL("brave_snap"))) {
    return base::unexpected("Failed to create snap temp directory");
  }
  const base::FilePath unpacked_dir =
      temp_dir.GetPath().AppendASCII("unpacked");

  std::string compressed;
  if (!base::ReadFileToStringWithMaxSize(tarball_path, &compressed,
                                         kMaxSnapTarballSize)) {
    return base::unexpected("Failed to read tarball from disk");
  }

  // GzipUncompress resizes its output buffer to the size recorded in the gzip
  // trailer before inflating, so check that bound first.
  if (compression::GetUncompressedSize(base::as_byte_span(compressed)) >
      kMaxSnapTarballSize) {
    return base::unexpected("Tarball is too large");
  }

  std::string decompressed;
  if (!compression::GzipUncompress(compressed, &decompressed)) {
    return base::unexpected("Failed to decompress tarball");
  }

  // Phase 1: extract snap.manifest.json.
  std::optional<std::string> manifest_json =
      ExtractFileFromTar(decompressed, "snap.manifest.json");
  if (!manifest_json) {
    return base::unexpected(
        "Failed to extract snap.manifest.json from tarball");
  }

  // Phase 2: the manifest must tell us which entry is the bundle. Guessing
  // would let a crafted archive choose the bundle for us.
  std::optional<base::DictValue> manifest =
      base::JSONReader::ReadDict(*manifest_json, base::JSON_PARSE_RFC);
  if (!manifest) {
    return base::unexpected("Failed to parse snap.manifest.json");
  }
  const std::string* bundle_file_path = nullptr;
  if (const auto* source = manifest->FindDict("source")) {
    if (const auto* location = source->FindDict("location")) {
      if (const auto* npm = location->FindDict("npm")) {
        bundle_file_path = npm->FindString("filePath");
      }
    }
  }
  if (!bundle_file_path || bundle_file_path->empty()) {
    return base::unexpected("Manifest is missing source.location.npm.filePath");
  }

  // Phase 3: extract bundle.
  std::optional<SnapTarResult> extracted =
      ExtractSnapFiles(decompressed, *bundle_file_path);
  if (!extracted) {
    return base::unexpected("Failed to extract snap bundle from tarball");
  }
  extracted->manifest_json = std::move(*manifest_json);

  // Compute MetaMask checksum before the in-memory content is released.
  std::optional<std::string> computed_shasum =
      SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
          decompressed, extracted->bundle_js, *bundle_file_path,
          extracted->manifest_json);
  if (!computed_shasum) {
    return base::unexpected("Failed to compute MetaMask checksum");
  }

  // Write extracted files to the unpacked directory.
  if (!base::CreateDirectory(unpacked_dir)) {
    return base::unexpected("Failed to create unpacked directory");
  }
  if (!base::WriteFile(unpacked_dir.AppendASCII("bundle.js"),
                       extracted->bundle_js)) {
    return base::unexpected("Failed to write bundle.js to unpacked directory");
  }
  if (!base::WriteFile(unpacked_dir.AppendASCII("manifest.json"),
                       extracted->manifest_json)) {
    return base::unexpected(
        "Failed to write manifest.json to unpacked directory");
  }

  SnapTarballExtractResult result;
  result.manifest_json = std::move(extracted->manifest_json);
  result.computed_shasum = std::move(*computed_shasum);
  result.bundle_size_bytes =
      base::checked_cast<uint64_t>(extracted->bundle_js.size());
  result.temp_dir_path = temp_dir.Take();
  return result;
}

}  // namespace brave_wallet

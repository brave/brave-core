/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_tar_decompressor.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_checksum_calculator.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_tar_utils.h"
#include "third_party/zlib/google/compression_utils.h"

namespace brave_wallet {

namespace {

SnapInstaller::ExtractResult ExtractError(std::string msg) {
  SnapInstaller::ExtractResult r;
  r.error = std::move(msg);
  return r;
}

}  // namespace

SnapInstaller::ExtractResult SnapInstallerTarDecompressor::ExtractTarballToDir(
    base::FilePath tarball_path) {
  // Create a snap-specific temp dir on the thread pool (blocking allowed here).
  base::FilePath temp_dir_path;
  if (!base::CreateNewTempDirectory(FILE_PATH_LITERAL("brave_snap"),
                                    &temp_dir_path)) {
    base::DeleteFile(tarball_path);
    return ExtractError("Failed to create snap temp directory");
  }
  const base::FilePath unpacked_dir = temp_dir_path.AppendASCII("unpacked");

  // Read and delete the downloaded tarball.
  std::string compressed;
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: reading tarball from "
             << tarball_path.value();
  if (!base::ReadFileToString(tarball_path, &compressed)) {
    base::DeleteFile(tarball_path);
    return ExtractError("Failed to read tarball from disk");
  }
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: tarball read compressed_size="
             << compressed.size();
  base::DeleteFile(tarball_path);

  // Gzip decompress.
  std::string decompressed;
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: gzip decompressing...";
  if (!compression::GzipUncompress(compressed, &decompressed)) {
    LOG(ERROR) << "XXXZZZ ExtractTarballToDir: gzip decompress FAILED";
    return ExtractError("Failed to decompress tarball");
  }
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: decompressed_size="
             << decompressed.size();

  // Phase 1: extract snap.manifest.json.
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: extracting snap.manifest.json";
  std::optional<std::string> manifest_json =
      ExtractFileFromTar(decompressed, "snap.manifest.json");
  if (!manifest_json) {
    LOG(ERROR) << "XXXZZZ ExtractTarballToDir: snap.manifest.json NOT FOUND";
    return ExtractError("Failed to extract snap.manifest.json from tarball");
  }
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: manifest_json='" << *manifest_json
             << "'";

  // Phase 2: parse manifest for bundle filePath.
  std::string bundle_file_path;
  std::string expected_shasum;
  {
    auto parsed = base::JSONReader::Read(*manifest_json, base::JSON_PARSE_RFC);
    if (parsed && parsed->is_dict()) {
      if (const auto* source = parsed->GetDict().FindDict("source")) {
        if (const auto* s = source->FindString("shasum")) {
          expected_shasum = *s;
        }
        if (const auto* location = source->FindDict("location")) {
          if (const auto* npm = location->FindDict("npm")) {
            if (const auto* fp = npm->FindString("filePath")) {
              bundle_file_path = *fp;
            }
          }
        }
      }
    }
  }
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: bundle_file_path='"
             << bundle_file_path << "' expected_shasum='" << expected_shasum
             << "'";

  // Phase 3: extract bundle.
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: extracting bundle files";
  std::optional<SnapTarResult> extracted =
      ExtractSnapFiles(decompressed, bundle_file_path);
  if (!extracted) {
    LOG(ERROR) << "XXXZZZ ExtractTarballToDir: bundle extraction FAILED";
    return ExtractError("Failed to extract snap bundle from tarball");
  }
  extracted->manifest_json = std::move(*manifest_json);
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: bundle_js_size="
             << extracted->bundle_js.size();

  // Compute MetaMask checksum before the in-memory content is released.
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: computing checksum...";
  std::string computed_shasum =
      SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
          decompressed, extracted->bundle_js, extracted->manifest_json);
  LOG(ERROR) << "XXXZZZ ExtractTarballToDir: shasum "
             << (computed_shasum == expected_shasum ? "MATCH" : "MISMATCH")
             << " expected='" << expected_shasum << "'"
             << " computed='" << computed_shasum << "'";

  // Write extracted files to the unpacked directory.
  if (!base::CreateDirectory(unpacked_dir)) {
    return ExtractError("Failed to create unpacked directory");
  }
  if (!base::WriteFile(unpacked_dir.AppendASCII("bundle.js"),
                       extracted->bundle_js)) {
    return ExtractError("Failed to write bundle.js to unpacked directory");
  }
  if (!base::WriteFile(unpacked_dir.AppendASCII("manifest.json"),
                       extracted->manifest_json)) {
    return ExtractError("Failed to write manifest.json to unpacked directory");
  }

  SnapInstaller::ExtractResult result;
  result.manifest_json = std::move(extracted->manifest_json);
  result.computed_shasum = std::move(computed_shasum);
  result.bundle_size_bytes = static_cast<uint64_t>(extracted->bundle_js.size());
  result.temp_dir_path = std::move(temp_dir_path);
  return result;
}

}  // namespace brave_wallet

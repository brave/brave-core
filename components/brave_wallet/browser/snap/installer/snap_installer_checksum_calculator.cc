/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_checksum_calculator.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_tar_utils.h"
#include "crypto/hash.h"

namespace brave_wallet {

namespace {

// MetaMask's getSnapChecksum() hashes the manifest with source.shasum removed,
// serialized by fast-json-stable-stringify, which sorts object keys. No
// explicit sort is needed here: base::DictValue stores entries in a flat_map
// ordered by std::less<std::string> and base::WriteJson emits them in that
// order. (The two orders diverge only for keys outside the BMP, where
// std::less compares UTF-8 bytes while JS '<' compares UTF-16 code units --
// snap manifest keys never are.)
std::optional<std::string> GetChecksummableManifestJson(
    base::DictValue manifest) {
  if (base::DictValue* source = manifest.FindDict("source")) {
    source->Remove("shasum");
  }
  return base::WriteJson(manifest);
}

}  // namespace

// Computes the MetaMask snap checksum:
//   base64( sha256( concat( sha256(file) for file in sorted_files ) ) )
//
// MetaMask's getSnapChecksum() collects the checksummable manifest, source
// bundle, optional icon, auxiliary files and localization files; sorts them by
// path; hashes each file separately; concatenates the 32-byte hashes; then
// hashes that concatenation.
//
// Reference: @metamask/snaps-utils getSnapChecksum() / checksumFiles()
std::optional<std::string>
SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
    const std::string& decompressed_tar,
    const std::string& bundle_js,
    const std::string& bundle_file_path,
    const std::string& manifest_json) {
  std::optional<base::DictValue> manifest =
      base::JSONReader::ReadDict(manifest_json, base::JSON_PARSE_RFC);
  if (!manifest) {
    return std::nullopt;
  }

  // Collect the auxiliary paths before the dict is consumed below.
  std::string icon_path;
  std::vector<std::string> other_paths;
  if (const auto* source = manifest->FindDict("source")) {
    if (const auto* loc = source->FindDict("location")) {
      if (const auto* npm = loc->FindDict("npm")) {
        if (const auto* ip = npm->FindString("iconPath")) {
          icon_path = *ip;
        }
      }
    }
    for (const char* key : {"files", "locales"}) {
      if (const auto* list = source->FindList(key)) {
        for (const auto& item : *list) {
          if (item.is_string()) {
            other_paths.push_back(item.GetString());
          }
        }
      }
    }
  }

  // Build the checksummable manifest (source.shasum removed).
  std::optional<std::string> checksummable_manifest =
      GetChecksummableManifestJson(std::move(*manifest));
  if (!checksummable_manifest) {
    return std::nullopt;
  }

  // Collect file paths and contents.
  std::vector<std::pair<std::string, std::string>> files;
  files.emplace_back(bundle_file_path, bundle_js);
  files.emplace_back("snap.manifest.json", *checksummable_manifest);

  if (!icon_path.empty()) {
    auto icon_data = ExtractFileFromTar(decompressed_tar, icon_path);
    if (!icon_data) {
      return std::nullopt;
    }
    files.emplace_back(icon_path, *icon_data);
  }

  for (const auto& path : other_paths) {
    auto file = ExtractFileFromTar(decompressed_tar, path);
    if (!file) {
      return std::nullopt;
    }
    files.emplace_back(path, *file);
  }

  // Sort by path. Bytewise order matches JS UTF-16 code-unit order for ASCII
  // paths, which is what MetaMask sorts on.
  std::sort(files.begin(), files.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  // Hash each file and concatenate the 32-byte digests.
  std::vector<uint8_t> concatenated_hashes;
  concatenated_hashes.reserve(files.size() * crypto::hash::kSha256Size);
  for (const auto& [path, content] : files) {
    auto hash = crypto::hash::Sha256(content);
    concatenated_hashes.insert(concatenated_hashes.end(), hash.begin(),
                               hash.end());
  }

  auto final_hash =
      crypto::hash::Sha256(base::as_byte_span(concatenated_hashes));
  return base::Base64Encode(final_hash);
}

}  // namespace brave_wallet

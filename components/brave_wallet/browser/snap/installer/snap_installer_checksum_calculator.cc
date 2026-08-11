/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_checksum_calculator.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
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
base::Value SortValue(const base::Value& value);

base::DictValue SortDictKeys(const base::DictValue& dict) {
  std::vector<std::string_view> keys;
  for (const auto [key, value] : dict) {
    keys.push_back(key);
  }
  std::sort(keys.begin(), keys.end());

  base::DictValue sorted;
  for (const std::string_view key : keys) {
    const base::Value* value = dict.Find(key);
    CHECK(value);
    sorted.Set(key, SortValue(*value));
  }
  return sorted;
}

base::Value SortValue(const base::Value& value) {
  if (value.is_dict()) {
    return base::Value(SortDictKeys(value.GetDict()));
  }
  if (value.is_list()) {
    base::ListValue sorted;
    for (const auto& item : value.GetList()) {
      sorted.Append(SortValue(item));
    }
    return base::Value(std::move(sorted));
  }
  return value.Clone();
}

// Returns the manifest JSON with source.shasum removed and keys sorted
// deterministically, matching MetaMask's fast-json-stable-stringify output.
std::optional<std::string> GetChecksummableManifestJson(
    const std::string& manifest_json) {
  auto parsed = base::JSONReader::Read(manifest_json, base::JSON_PARSE_RFC);
  if (!parsed || !parsed->is_dict()) {
    return std::nullopt;
  }

  base::Value sorted = SortValue(*parsed);
  if (!sorted.is_dict()) {
    return std::nullopt;
  }

  if (base::DictValue* source = sorted.GetDict().FindDict("source")) {
    source->Remove("shasum");
  }

  return base::WriteJson(sorted);
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
  // Build the checksummable manifest (source.shasum removed, keys sorted).
  std::optional<std::string> checksummable_manifest =
      GetChecksummableManifestJson(manifest_json);
  if (!checksummable_manifest) {
    return std::nullopt;
  }

  // Collect file paths and contents.
  std::vector<std::pair<std::string, std::string>> files;
  files.emplace_back(bundle_file_path, bundle_js);
  files.emplace_back("snap.manifest.json", *checksummable_manifest);

  std::string icon_path;
  std::vector<std::string> other_paths;
  {
    auto parsed = base::JSONReader::Read(manifest_json, base::JSON_PARSE_RFC);
    if (parsed && parsed->is_dict()) {
      if (const auto* source = parsed->GetDict().FindDict("source")) {
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
    }
  }

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

  // Sort by path (UTF-16 code unit order; ASCII lexicographic is equivalent).
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

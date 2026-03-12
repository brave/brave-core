/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_installer_checksum_calculator.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/snap_tar_utils.h"
#include "crypto/hash.h"

namespace brave_wallet {

namespace {

// Logs the hex of the first |n| bytes of |data| for debugging.
std::string HexPrefix(std::string_view data, size_t n = 24) {
  size_t len = std::min(n, data.size());
  return base::HexEncode(base::as_byte_span(data).first(len));
}

}  // namespace

// Computes the MetaMask snap checksum:
//   base64( sha256( sha256(bundle) || sha256(icon)? || sha256(aux/locale)* ) )
//
// MetaMask hashes each file separately, concatenates the 32-byte hashes in
// order [bundle, icon, sorted(aux+locales)], then hashes that concatenation.
// For a source-code-only snap this reduces to base64(sha256(sha256(bundle))).
//
// Reference: @metamask/snaps-utils checksumFiles()
std::string SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
    const std::string& decompressed_tar,
    const std::string& bundle_js,
    const std::string& manifest_json) {
  // Parse manifest for optional extra files.
  std::string icon_path;
  std::vector<std::string> other_paths;  // aux + locale files, sorted together

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

  std::sort(other_paths.begin(), other_paths.end());

  LOG(ERROR) << "SNAP checksum: bundle size=" << bundle_js.size()
             << " prefix=" << HexPrefix(bundle_js)
             << " suffix=" << HexPrefix(bundle_js.substr(
                                bundle_js.size() > 24 ? bundle_js.size() - 24
                                                      : 0));

  std::optional<std::string> icon_data;
  if (!icon_path.empty()) {
    icon_data = ExtractFileFromTar(decompressed_tar, icon_path);
    if (icon_data) {
      LOG(ERROR) << "SNAP checksum: icon size=" << icon_data->size()
                 << " prefix=" << HexPrefix(*icon_data);
    } else {
      LOG(ERROR) << "SNAP checksum: icon NOT FOUND: " << icon_path;
    }
  }

  // Always log bundle-only hash.
  LOG(ERROR) << "SNAP try: sha256(bundle)="
             << base::Base64Encode(crypto::hash::Sha256(bundle_js));

  // Variant: sha256(icon || bundle)
  if (icon_data) {
    std::string icon_first;
    icon_first.reserve(icon_data->size() + bundle_js.size());
    icon_first.append(*icon_data);
    icon_first.append(bundle_js);
    LOG(ERROR) << "SNAP try: sha256(icon||bundle)="
               << base::Base64Encode(crypto::hash::Sha256(icon_first));
  }

  // Build the canonical concatenation: bundle || icon? || sorted(aux+locale)
  std::string concat;
  concat.reserve(bundle_js.size() + (icon_data ? icon_data->size() : 0));
  concat.append(bundle_js);
  if (icon_data) {
    concat.append(*icon_data);
  }
  for (const auto& path : other_paths) {
    auto file = ExtractFileFromTar(decompressed_tar, path);
    if (file) {
      concat.append(*file);
      LOG(ERROR) << "SNAP checksum: appended aux '" << path
                 << "' size=" << file->size();
    } else {
      LOG(ERROR) << "SNAP checksum: aux not found: " << path;
    }
  }

  auto final_hash = crypto::hash::Sha256(concat);
  std::string result = base::Base64Encode(final_hash);
  LOG(ERROR) << "SNAP checksum: sha256(bundle" << (icon_data ? "||icon" : "")
             << ")=" << result << " total_bytes=" << concat.size();
  return result;
}

}  // namespace brave_wallet

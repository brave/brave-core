/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/tar_test_helpers.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/span.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "crypto/hash.h"
#include "third_party/zlib/google/compression_utils.h"

namespace brave_wallet {

namespace {

// POSIX ustar header layout (must match snap_tar_utils.cc).
constexpr size_t kBlockSize = 512;

// Writes |value| as (width - 1) zero-padded octal digits followed by a NUL
// terminator into |buf| starting at |offset|.
void SetOctalField(std::string& buf,
                   size_t offset,
                   size_t width,
                   uint64_t value) {
  std::string oct;
  do {
    oct.push_back(static_cast<char>('0' + (value & 7)));
    value >>= 3;
  } while (value != 0);
  std::reverse(oct.begin(), oct.end());

  const size_t digits = width - 1;
  CHECK_LE(oct.size(), digits);
  std::string field(digits - oct.size(), '0');
  field += oct;
  for (size_t i = 0; i < digits; ++i) {
    buf[offset + i] = field[i];
  }
  buf[offset + width - 1] = '\0';
}

// Computes the ustar header checksum over |header| (with the checksum field
// treated as spaces) and writes it back as "NNNNNN\0 ".
void SetChecksumField(std::string& header) {
  for (size_t i = 148; i < 156; ++i) {
    header[i] = ' ';
  }
  uint32_t sum = 0;
  for (char c : header) {
    sum += static_cast<unsigned char>(c);
  }
  std::string oct;
  do {
    oct.push_back(static_cast<char>('0' + (sum & 7)));
    sum >>= 3;
  } while (sum != 0);
  std::reverse(oct.begin(), oct.end());

  CHECK_LE(oct.size(), 6u);
  std::string field(6 - oct.size(), '0');
  field += oct;
  for (size_t i = 0; i < 6; ++i) {
    header[148 + i] = field[i];
  }
  header[154] = '\0';
  header[155] = ' ';
}

}  // namespace

std::string BuildUstarTar(
    const std::vector<std::pair<std::string, std::string>>& entries) {
  std::string out;
  for (const auto& [path, content] : entries) {
    CHECK_LE(path.size(), 100u) << "ustar name field overflow: " << path;
    std::string header(kBlockSize, '\0');
    std::copy(path.begin(), path.end(), header.begin());  // name @ 0
    SetOctalField(header, 100, 8, 0644);                    // mode
    SetOctalField(header, 108, 8, 0);                     // uid
    SetOctalField(header, 116, 8, 0);                     // gid
    SetOctalField(header, 124, 12, content.size());     // size
    SetOctalField(header, 136, 12, 0);                  // mtime
    header[156] = '0';                                  // typeflag: regular
    std::string_view ustar_magic("ustar");
    std::copy(ustar_magic.begin(), ustar_magic.end(),
              header.begin() + 257);  // magic @ 257
    header[263] = '0';                // version @ 263 ("00")
    header[264] = '0';
    SetChecksumField(header);
    out += header;

    out += content;
    const size_t remainder = content.size() % kBlockSize;
    if (remainder != 0) {
      out.append(kBlockSize - remainder, '\0');
    }
  }
  out.append(kBlockSize * 2, '\0');  // Two zero blocks: end-of-archive marker.
  return out;
}

std::string BuildSnapTar(const std::string& manifest_json,
                         const std::string& bundle_js,
                         const std::string& bundle_file_path) {
  return BuildUstarTar({
      {"package/snap.manifest.json", manifest_json},
      {"package/" + bundle_file_path, bundle_js},
  });
}

std::string GzipCompressForTest(const std::string& data) {
  std::string compressed;
  CHECK(compression::GzipCompress(base::as_byte_span(data), &compressed));
  return compressed;
}

std::string BuildSnapTarball(const std::string& manifest_json,
                             const std::string& bundle_js,
                             const std::string& bundle_file_path) {
  return GzipCompressForTest(
      BuildSnapTar(manifest_json, bundle_js, bundle_file_path));
}

std::string ComputeSnapBundleShasum(const std::string& bundle_js) {
  return base::Base64Encode(crypto::hash::Sha256(bundle_js));
}

std::string MakeMinimalSnapManifestJson(
    const std::string& bundle_file_path,
    const std::string& shasum) {
  base::DictValue npm;
  npm.Set("filePath", bundle_file_path);
  base::DictValue location;
  location.Set("npm", std::move(npm));
  base::DictValue source;
  source.Set("shasum", shasum);
  source.Set("location", std::move(location));

  base::DictValue manifest;
  manifest.Set("proposedName", "Test Snap");
  manifest.Set("description", "A snap used in tests");
  manifest.Set("source", std::move(source));
  manifest.Set("initialPermissions", base::DictValue());
  return base::WriteJson(manifest).value_or("{}");
}

}  // namespace brave_wallet

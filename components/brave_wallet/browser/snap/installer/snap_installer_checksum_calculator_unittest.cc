/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_checksum_calculator.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_tar_utils.h"
#include "brave/components/brave_wallet/browser/snap/installer/tar_test_helpers.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/zlib/google/compression_utils.h"

namespace brave_wallet {

namespace {

// Builds a minimal snap.manifest.json with the given bundle path and optional
// icon path / auxiliary file paths.
std::string MakeManifestJson(const std::string& bundle_file_path,
                             const std::string& shasum,
                             const std::string& icon_path = "",
                             const std::vector<std::string>& files = {}) {
  std::string json = "{\"proposedName\":\"Test Snap\",";
  json += "\"description\":\"A snap used in tests\",";
  json += "\"source\":{\"shasum\":\"" + shasum + "\",";
  json += "\"location\":{\"npm\":{\"filePath\":\"" + bundle_file_path + "\"";
  if (!icon_path.empty()) {
    json += ",\"iconPath\":\"" + icon_path + "\"";
  }
  json += "}}";
  if (!files.empty()) {
    json += ",\"files\":[";
    for (size_t i = 0; i < files.size(); ++i) {
      if (i > 0) {
        json += ",";
      }
      json += "\"" + files[i] + "\"";
    }
    json += "]";
  }
  json += "},\"initialPermissions\":{}}";
  return json;
}

std::string BuildSnapTarWithFiles(
    const std::string& manifest_json,
    const std::string& bundle_js,
    const std::string& bundle_file_path,
    const std::vector<std::pair<std::string, std::string>>& extra_files = {}) {
  std::vector<std::pair<std::string, std::string>> entries = {
      {"package/snap.manifest.json", manifest_json},
      {"package/" + bundle_file_path, bundle_js},
  };
  entries.insert(entries.end(), extra_files.begin(), extra_files.end());
  return BuildUstarTar(entries);
}

}  // namespace

TEST(SnapInstallerChecksumCalculatorTest, SourceOnlySnap) {
  const std::string bundle = "export const onRpcRequest = () => 42;";
  const std::string manifest =
      MakeManifestJson("dist/bundle.js", ComputeSnapBundleShasum(bundle));
  std::string tar = BuildSnapTarWithFiles(manifest, bundle, "dist/bundle.js");

  std::optional<std::string> checksum =
      SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
          tar, bundle, "dist/bundle.js", manifest);

  // Expected value computed independently from MetaMask's getSnapChecksum:
  // files sorted as [dist/bundle.js, snap.manifest.json], each hashed, hashes
  // concatenated and hashed, then base64 encoded.
  ASSERT_TRUE(checksum);
  EXPECT_EQ(*checksum, "1PJusGW80ttgzxknJidI/JKg2Timv0O/NV6b57TKWTI=");
}

TEST(SnapInstallerChecksumCalculatorTest, RealArchiveMatchesManifestShasum) {
  base::FilePath real_tarball =
      BraveWalletComponentsTestDataFolder()
          .AppendASCII("snap_installer")
          .AppendASCII("name-lookup-example-snap-3.1.2.tgz");

  base::ScopedAllowBlockingForTesting allow_blocking;
  std::string compressed;
  ASSERT_TRUE(base::ReadFileToString(real_tarball, &compressed));

  std::string decompressed;
  ASSERT_TRUE(compression::GzipUncompress(compressed, &decompressed));

  std::string manifest_json;
  {
    auto manifest = ExtractFileFromTar(decompressed, "snap.manifest.json");
    ASSERT_TRUE(manifest);
    manifest_json = std::move(*manifest);
  }

  std::string bundle_js;
  {
    auto bundle = ExtractFileFromTar(decompressed, "dist/bundle.js");
    ASSERT_TRUE(bundle);
    bundle_js = std::move(*bundle);
  }

  std::optional<std::string> checksum =
      SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
          decompressed, bundle_js, "dist/bundle.js", manifest_json);

  // The published manifest's source.shasum is the canonical MetaMask checksum.
  ASSERT_TRUE(checksum);
  EXPECT_EQ(*checksum, "SRmLTMVKJvWHyVH5H3HhvMz0iQOWn4St4/9xX8Sv1EM=");
}

TEST(SnapInstallerChecksumCalculatorTest, SnapWithIcon) {
  const std::string bundle = "export const onRpcRequest = () => 42;";
  const std::string icon = "<svg></svg>";
  const std::string manifest = MakeManifestJson(
      "dist/bundle.js", ComputeSnapBundleShasum(bundle), "images/icon.svg");
  std::string tar = BuildSnapTarWithFiles(manifest, bundle, "dist/bundle.js",
                                          {{"package/images/icon.svg", icon}});

  std::optional<std::string> checksum =
      SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
          tar, bundle, "dist/bundle.js", manifest);

  // Expected value: files sorted as [dist/bundle.js, images/icon.svg,
  // snap.manifest.json], each hashed, concatenated and hashed.
  ASSERT_TRUE(checksum);
  EXPECT_EQ(*checksum, "Gr0LUU7eFajYd8H+Z+AleG5QuSwHx8mETGBFOUlAB7Q=");
}

TEST(SnapInstallerChecksumCalculatorTest, SnapWithAuxFiles) {
  const std::string bundle = "export const onRpcRequest = () => 42;";
  const std::string aux1 = "aux1 content";
  const std::string aux2 = "aux2 content";
  const std::string manifest =
      MakeManifestJson("dist/bundle.js", ComputeSnapBundleShasum(bundle), "",
                       {"z-aux.txt", "a-aux.txt"});
  std::string tar = BuildSnapTarWithFiles(
      manifest, bundle, "dist/bundle.js",
      {{"package/a-aux.txt", aux1}, {"package/z-aux.txt", aux2}});

  std::optional<std::string> checksum =
      SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
          tar, bundle, "dist/bundle.js", manifest);

  // Expected value: auxiliary files are sorted by path before hashing.
  ASSERT_TRUE(checksum);
  EXPECT_EQ(*checksum, "R9fUsLqn6Vly3t3JnVqDLvH1MpUf8TJ+anABqPRAerY=");
}

}  // namespace brave_wallet

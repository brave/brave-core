/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_checksum_calculator.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/extend.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_tar_utils.h"
#include "brave/components/brave_wallet/browser/snap/installer/tar_test_helpers.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/zlib/google/compression_utils.h"

namespace brave_wallet {

namespace {

// Builds a minimal snap.manifest.json with the given bundle path and optional
// icon path / auxiliary file paths.
std::string MakeManifestJson(std::string_view bundle_file_path,
                             std::string_view shasum,
                             std::string_view icon_path = "",
                             const std::vector<std::string>& files = {},
                             std::string_view description =
                                 "A snap used in tests") {
  base::DictValue npm;
  npm.Set("filePath", bundle_file_path);
  if (!icon_path.empty()) {
    npm.Set("iconPath", icon_path);
  }
  base::DictValue location;
  location.Set("npm", std::move(npm));

  base::DictValue source;
  source.Set("shasum", shasum);
  source.Set("location", std::move(location));
  if (!files.empty()) {
    base::ListValue file_list;
    for (const auto& file : files) {
      file_list.Append(file);
    }
    source.Set("files", std::move(file_list));
  }

  base::DictValue manifest;
  manifest.Set("proposedName", "Test Snap");
  manifest.Set("description", description);
  manifest.Set("source", std::move(source));
  manifest.Set("initialPermissions", base::DictValue());
  return base::WriteJson(manifest).value();
}

std::string BuildSnapTarWithFiles(
    std::string_view manifest_json,
    std::string_view bundle_js,
    std::string_view bundle_file_path,
    const std::vector<std::pair<std::string, std::string>>& extra_files = {}) {
  std::vector<std::pair<std::string, std::string>> entries = {
      {"package/snap.manifest.json", std::string(manifest_json)},
      {base::StrCat({"package/", bundle_file_path}), std::string(bundle_js)},
  };
  base::Extend(entries, extra_files);
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

TEST(SnapInstallerChecksumCalculatorTest,
     ManifestKeyOrderDoesNotAffectChecksum) {
  const std::string bundle = "export const onRpcRequest = () => 42;";
  const std::string shasum = ComputeSnapBundleShasum(bundle);
  // Deliberately a raw literal rather than base::WriteJson: this test asserts
  // key order does not affect the checksum, and base::DictValue would sort the
  // keys before serialization, erasing what is under test.
  const std::string reordered =
      "{\"source\":{\"location\":{\"npm\":{\"filePath\":\"dist/bundle.js\"}},"
      "\"shasum\":\"" +
      shasum +
      "\"},"
      "\"initialPermissions\":{},"
      "\"description\":\"A snap used in tests\","
      "\"proposedName\":\"Test Snap\"}";
  std::string tar = BuildSnapTarWithFiles(reordered, bundle, "dist/bundle.js");

  auto checksum = SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
      tar, bundle, "dist/bundle.js", reordered);
  ASSERT_TRUE(checksum);
  // Identical to SourceOnlySnap, which spells the same keys alphabetically.
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

// TODO(https://github.com/brave/brave-core/issues/39185): base::WriteJson is
// not a drop-in for JS JSON.stringify, which MetaMask's getSnapChecksum() uses
// via fast-json-stable-stringify. base::EscapeJSONString escapes '<' as a
// \u sequence, and likewise U+2028/U+2029 (base/json/string_escape.cc);
// JSON.stringify emits all three literally. A snap whose manifest contains them
// therefore gets a checksum MetaMask disagrees with, and installation fails
// closed. A JSON.stringify-compatible serializer is needed before snaps ship;
// this test pins the current, divergent behavior so the fix is detectable.
TEST(SnapInstallerChecksumCalculatorTest, AngleBracketDivergesFromMetaMask) {
  const std::string bundle = "export const onRpcRequest = () => 42;";
  const std::string manifest = MakeManifestJson(
      "dist/bundle.js", ComputeSnapBundleShasum(bundle), "", {},
      "A <3 snap used in tests");
  ASSERT_NE(manifest.find("\\u003C"), std::string::npos)
      << "base::WriteJson stopped escaping '<'; revisit this TODO";
  std::string tar = BuildSnapTarWithFiles(manifest, bundle, "dist/bundle.js");

  std::optional<std::string> checksum =
      SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(
          tar, bundle, "dist/bundle.js", manifest);

  ASSERT_TRUE(checksum);
  // What we currently compute, from a manifest whose description serializes as
  // "A <3 snap used in tests". MetaMask, serializing the same snap with a
  // literal '<', gets "bYh03590AfbX1G5p5+zP4VI9SBTMmv2o7h0aliUZ0Hc=". Once a
  // JSON.stringify-compatible serializer lands, that becomes the expectation.
  EXPECT_EQ(*checksum, "Iy/AVwMhxZ9umzheQU012mWwlDYpBSjgyMxFaLPJNl8=");
}

}  // namespace brave_wallet

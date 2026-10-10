/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_tar_decompressor.h"

#include <cstdint>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_wallet/browser/snap/installer/tar_test_helpers.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SnapInstallerTarDecompressorTest : public testing::Test {
 public:
  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }

 protected:
  base::FilePath WriteTarball(const std::string& bytes) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath path = temp_dir_.GetPath().AppendASCII("snap.tgz");
    CHECK(base::WriteFile(path, bytes));
    return path;
  }

  base::ScopedTempDir temp_dir_;
};

TEST_F(SnapInstallerTarDecompressorTest, ExtractsValidTarball) {
  const std::string bundle = "export const onRpcRequest = () => 42;";
  const std::string manifest = MakeMinimalSnapManifestJson(
      "dist/bundle.js", ComputeSnapBundleShasum(bundle));
  base::FilePath tarball =
      WriteTarball(BuildSnapTarball(manifest, bundle, "dist/bundle.js"));

  base::ScopedAllowBlockingForTesting allow_blocking;
  SnapTarballExtractOutcome result =
      SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);

  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(result->manifest_json, manifest);
  EXPECT_EQ(result->computed_shasum,
            "1PJusGW80ttgzxknJidI/JKg2Timv0O/NV6b57TKWTI=");
  EXPECT_EQ(result->bundle_size_bytes, bundle.size());
  ASSERT_FALSE(result->temp_dir_path.empty());

  const base::FilePath unpacked_dir =
      result->temp_dir_path.AppendASCII("unpacked");
  std::string extracted_bundle;
  ASSERT_TRUE(base::ReadFileToString(unpacked_dir.AppendASCII("bundle.js"),
                                     &extracted_bundle));
  EXPECT_EQ(extracted_bundle, bundle);
  std::string extracted_manifest;
  ASSERT_TRUE(base::ReadFileToString(unpacked_dir.AppendASCII("manifest.json"),
                                     &extracted_manifest));
  EXPECT_EQ(extracted_manifest, manifest);

  // The caller owns the downloaded tarball; extraction must not consume it.
  EXPECT_TRUE(base::PathExists(tarball));

  base::DeletePathRecursively(result->temp_dir_path);
}

TEST_F(SnapInstallerTarDecompressorTest, CorruptGzipReturnsError) {
  // The last 4 bytes are read as the gzip ISIZE trailer regardless of
  // validity; keep them small so this exercises the decompression failure
  // rather than the size-cap check.
  base::FilePath tarball = WriteTarball(std::string("not gzip\0\0\0\0", 12));
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Failed to decompress tarball");
}

TEST_F(SnapInstallerTarDecompressorTest, MissingManifestReturnsError) {
  std::string tar = BuildUstarTar({{"package/dist/bundle.js", "B"}});
  base::FilePath tarball = WriteTarball(GzipCompressForTest(tar));
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "Failed to extract snap.manifest.json from tarball");
}

TEST_F(SnapInstallerTarDecompressorTest, MissingBundleReturnsError) {
  const std::string manifest = MakeMinimalSnapManifestJson();
  // Manifest present, but the declared bundle path is absent from the archive.
  std::string tar = BuildUstarTar({{"package/snap.manifest.json", manifest}});
  base::FilePath tarball = WriteTarball(GzipCompressForTest(tar));
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Failed to extract snap bundle from tarball");
}

TEST_F(SnapInstallerTarDecompressorTest, UnparseableManifestReturnsError) {
  std::string tar = BuildUstarTar({
      {"package/snap.manifest.json", "not-json"},
      {"package/dist/bundle.js", "B"},
  });
  base::FilePath tarball = WriteTarball(GzipCompressForTest(tar));
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Failed to parse snap.manifest.json");
}

TEST_F(SnapInstallerTarDecompressorTest,
       ManifestWithoutBundlePathReturnsError) {
  // No source.location.npm.filePath: the archive contains a plausible-looking
  // dist/*.js that must not be picked up as the bundle.
  constexpr char kManifest[] = R"({"proposedName":"Test Snap","source":{}})";
  std::string tar = BuildUstarTar({
      {"package/snap.manifest.json", kManifest},
      {"package/dist/bundle.js", "B"},
  });
  base::FilePath tarball = WriteTarball(GzipCompressForTest(tar));
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Manifest is missing source.location.npm.filePath");
}

TEST_F(SnapInstallerTarDecompressorTest,
       OversizedDecompressedSizeReturnsError) {
  const std::string bundle = "export const onRpcRequest = () => 42;";
  std::string gz =
      BuildSnapTarball(MakeMinimalSnapManifestJson(
                           "dist/bundle.js", ComputeSnapBundleShasum(bundle)),
                       bundle, "dist/bundle.js");
  // The last 4 bytes of a gzip stream are the little-endian uncompressed size,
  // which is the value GzipUncompress preallocates from.
  ASSERT_GE(gz.size(), 4u);
  const uint32_t forged_size = kMaxSnapTarballSize + 1;
  for (size_t i = 0; i < 4; ++i) {
    gz[gz.size() - 4 + i] = static_cast<char>((forged_size >> (8 * i)) & 0xFF);
  }

  base::FilePath tarball = WriteTarball(gz);
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Tarball is too large");
}

TEST_F(SnapInstallerTarDecompressorTest, ExtractsRealNpmSnapTarball) {
  base::FilePath real_tarball =
      BraveWalletComponentsTestDataFolder()
          .AppendASCII("snap_installer")
          .AppendASCII("name-lookup-example-snap-3.1.2.tgz");

  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath tarball = temp_dir_.GetPath().AppendASCII("real.tgz");
  ASSERT_TRUE(base::CopyFile(real_tarball, tarball));

  SnapTarballExtractOutcome result =
      SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_THAT(result->manifest_json,
              testing::HasSubstr("Name Lookup Example Snap"));
  ASSERT_FALSE(result->temp_dir_path.empty());

  const base::FilePath unpacked_dir =
      result->temp_dir_path.AppendASCII("unpacked");
  EXPECT_TRUE(base::PathExists(unpacked_dir.AppendASCII("bundle.js")));
  EXPECT_TRUE(base::PathExists(unpacked_dir.AppendASCII("manifest.json")));

  // Verify the extracted bundle is intact and the computed shasum matches it.
  std::string extracted_bundle;
  ASSERT_TRUE(base::ReadFileToString(unpacked_dir.AppendASCII("bundle.js"),
                                     &extracted_bundle));
  EXPECT_EQ(result->bundle_size_bytes, extracted_bundle.size());
  EXPECT_EQ(result->computed_shasum,
            "SRmLTMVKJvWHyVH5H3HhvMz0iQOWn4St4/9xX8Sv1EM=");

  base::DeletePathRecursively(result->temp_dir_path);
}

}  // namespace brave_wallet

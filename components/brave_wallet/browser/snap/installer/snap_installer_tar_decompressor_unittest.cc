/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_tar_decompressor.h"

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
  SnapTarballExtractResult result =
      SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);

  EXPECT_TRUE(result.error.empty()) << result.error;
  EXPECT_EQ(result.manifest_json, manifest);
  EXPECT_EQ(result.computed_shasum, ComputeSnapBundleShasum(bundle));
  EXPECT_EQ(result.bundle_size_bytes, bundle.size());
  ASSERT_FALSE(result.temp_dir_path.empty());
  EXPECT_TRUE(base::PathExists(
      result.temp_dir_path.AppendASCII("unpacked").AppendASCII("bundle.js")));
  EXPECT_TRUE(base::PathExists(result.temp_dir_path.AppendASCII("unpacked")
                                   .AppendASCII("manifest.json")));
  // The downloaded tarball is consumed (deleted) by extraction.
  EXPECT_FALSE(base::PathExists(tarball));

  base::DeletePathRecursively(result.temp_dir_path);
}

TEST_F(SnapInstallerTarDecompressorTest, CorruptGzipReturnsError) {
  base::FilePath tarball = WriteTarball("this is not gzip data");
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  EXPECT_EQ(result.error, "Failed to decompress tarball");
}

TEST_F(SnapInstallerTarDecompressorTest, MissingManifestReturnsError) {
  std::string tar = BuildUstarTar({{"package/dist/bundle.js", "B"}});
  base::FilePath tarball = WriteTarball(GzipCompressForTest(tar));
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  EXPECT_EQ(result.error, "Failed to extract snap.manifest.json from tarball");
}

TEST_F(SnapInstallerTarDecompressorTest, MissingBundleReturnsError) {
  const std::string manifest = MakeMinimalSnapManifestJson();
  // Manifest present, but the declared bundle path is absent from the archive.
  std::string tar = BuildUstarTar({{"package/snap.manifest.json", manifest}});
  base::FilePath tarball = WriteTarball(GzipCompressForTest(tar));
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  EXPECT_EQ(result.error, "Failed to extract snap bundle from tarball");
}

TEST_F(SnapInstallerTarDecompressorTest, ExtractsRealNpmSnapTarball) {
  base::FilePath real_tarball =
      BraveWalletComponentsTestDataFolder()
          .AppendASCII("snap_installer")
          .AppendASCII("name-lookup-example-snap-3.1.2.tgz");

  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath tarball = temp_dir_.GetPath().AppendASCII("real.tgz");
  ASSERT_TRUE(base::CopyFile(real_tarball, tarball));

  SnapTarballExtractResult result =
      SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  EXPECT_TRUE(result.error.empty()) << result.error;
  EXPECT_THAT(result.manifest_json,
              testing::HasSubstr("Name Lookup Example Snap"));
  ASSERT_FALSE(result.temp_dir_path.empty());

  const base::FilePath unpacked_dir =
      result.temp_dir_path.AppendASCII("unpacked");
  EXPECT_TRUE(base::PathExists(unpacked_dir.AppendASCII("bundle.js")));
  EXPECT_TRUE(base::PathExists(unpacked_dir.AppendASCII("manifest.json")));

  // Verify the extracted bundle is intact and the computed shasum matches it.
  std::string extracted_bundle;
  ASSERT_TRUE(
      base::ReadFileToString(unpacked_dir.AppendASCII("bundle.js"),
                             &extracted_bundle));
  EXPECT_EQ(result.bundle_size_bytes, extracted_bundle.size());
  EXPECT_EQ(result.computed_shasum,
            ComputeSnapBundleShasum(extracted_bundle));

  base::DeletePathRecursively(result.temp_dir_path);
}

}  // namespace brave_wallet

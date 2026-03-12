/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_tar_decompressor.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_installer.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
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
  TestSnapManifestOptions options;
  options.shasum = ComputeSnapBundleShasum(bundle);
  const std::string manifest = MakeSnapManifestJson(options);
  base::FilePath tarball =
      WriteTarball(BuildSnapTarball(manifest, bundle, options.bundle_file_path));

  base::ScopedAllowBlockingForTesting allow_blocking;
  SnapInstaller::ExtractResult result =
      SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);

  EXPECT_TRUE(result.error.empty()) << result.error;
  EXPECT_EQ(result.manifest_json, manifest);
  EXPECT_EQ(result.computed_shasum, ComputeSnapBundleShasum(bundle));
  EXPECT_EQ(result.bundle_size_bytes, bundle.size());
  ASSERT_FALSE(result.temp_dir_path.empty());
  EXPECT_TRUE(base::PathExists(
      result.temp_dir_path.AppendASCII("unpacked").AppendASCII("bundle.js")));
  EXPECT_TRUE(base::PathExists(
      result.temp_dir_path.AppendASCII("unpacked").AppendASCII("manifest.json")));
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
  const std::string manifest = MakeSnapManifestJson(TestSnapManifestOptions());
  // Manifest present, but the declared bundle path is absent from the archive.
  std::string tar = BuildUstarTar({{"package/snap.manifest.json", manifest}});
  base::FilePath tarball = WriteTarball(GzipCompressForTest(tar));
  base::ScopedAllowBlockingForTesting allow_blocking;
  auto result = SnapInstallerTarDecompressor::ExtractTarballToDir(tarball);
  EXPECT_EQ(result.error, "Failed to extract snap bundle from tarball");
}

}  // namespace brave_wallet

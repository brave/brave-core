/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_tar_utils.h"

#include <string>

#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SnapTarUtilsTest, ExtractFileFromTarBySuffix) {
  std::string tar = BuildUstarTar({
      {"package/snap.manifest.json", "MANIFEST"},
      {"package/dist/bundle.js", "BUNDLE"},
  });

  auto manifest = ExtractFileFromTar(tar, "snap.manifest.json");
  ASSERT_TRUE(manifest);
  EXPECT_EQ(*manifest, "MANIFEST");

  auto bundle = ExtractFileFromTar(tar, "dist/bundle.js");
  ASSERT_TRUE(bundle);
  EXPECT_EQ(*bundle, "BUNDLE");

  // A shorter suffix of the full path still matches.
  auto by_short_suffix = ExtractFileFromTar(tar, "bundle.js");
  ASSERT_TRUE(by_short_suffix);
  EXPECT_EQ(*by_short_suffix, "BUNDLE");
}

TEST(SnapTarUtilsTest, ExtractFileFromTarIsCaseSensitive) {
  std::string tar = BuildUstarTar({{"package/snap.manifest.json", "X"}});
  EXPECT_FALSE(ExtractFileFromTar(tar, "SNAP.MANIFEST.JSON"));
}

TEST(SnapTarUtilsTest, ExtractFileFromTarMissingFile) {
  std::string tar = BuildUstarTar({{"package/snap.manifest.json", "X"}});
  EXPECT_FALSE(ExtractFileFromTar(tar, "nonexistent.txt"));
}

TEST(SnapTarUtilsTest, ExtractFileFromTarMalformedReturnsNullopt) {
  std::string tar = BuildUstarTar({{"package/snap.manifest.json", "HELLO"}});
  // The size field of the first header begins at offset 124; a non-octal digit
  // makes ParseOctal fail and the whole archive parse abort.
  tar[124] = '9';
  EXPECT_FALSE(ExtractFileFromTar(tar, "snap.manifest.json"));
}

TEST(SnapTarUtilsTest, ExtractSnapFilesWithExplicitBundlePath) {
  std::string tar = BuildSnapTar("MANIFEST", "BUNDLE", "dist/bundle.js");
  auto result = ExtractSnapFiles(tar, "dist/bundle.js");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->manifest_json, "MANIFEST");
  EXPECT_EQ(result->bundle_js, "BUNDLE");
}

TEST(SnapTarUtilsTest, ExtractSnapFilesFallsBackToDistJs) {
  std::string tar = BuildUstarTar({
      {"package/snap.manifest.json", "MANIFEST"},
      {"package/dist/index.js", "BUNDLE"},
  });
  auto result = ExtractSnapFiles(tar, /*bundle_file_path=*/"");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->manifest_json, "MANIFEST");
  EXPECT_EQ(result->bundle_js, "BUNDLE");
}

TEST(SnapTarUtilsTest, ExtractSnapFilesMissingManifest) {
  std::string tar = BuildUstarTar({{"package/dist/bundle.js", "BUNDLE"}});
  EXPECT_FALSE(ExtractSnapFiles(tar, "dist/bundle.js"));
}

TEST(SnapTarUtilsTest, ExtractSnapFilesMissingBundle) {
  std::string tar = BuildUstarTar({{"package/snap.manifest.json", "MANIFEST"}});
  EXPECT_FALSE(ExtractSnapFiles(tar, "dist/bundle.js"));
}

TEST(SnapTarUtilsTest, ExtractSnapFilesTruncatedReturnsNullopt) {
  std::string tar =
      BuildSnapTar("MANIFEST_LONG_ENOUGH", "BUNDLE", "dist/bundle.js");
  // Keep the first header but cut the file content short so the declared size
  // runs past the end of the buffer.
  tar.resize(512 + 4);
  EXPECT_FALSE(ExtractSnapFiles(tar, "dist/bundle.js"));
}

}  // namespace brave_wallet

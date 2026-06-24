/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_checksum_calculator.h"

#include <string>

#include "base/base64.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "crypto/hash.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// A manifest with no iconPath / files / locales, so the checksum reduces to
// the bundle hash alone.
constexpr char kSourceOnlyManifest[] =
    R"({"source": {"shasum": "x",
        "location": {"npm": {"filePath": "dist/bundle.js"}}}})";

std::string Checksum(const std::string& tar,
                     const std::string& bundle,
                     const std::string& manifest) {
  return SnapInstallerChecksumCalculator::ComputeMetaMaskChecksum(tar, bundle,
                                                                  manifest);
}

}  // namespace

TEST(SnapInstallerChecksumCalculatorTest, SourceOnlyChecksumIsBundleHash) {
  const std::string bundle = "console.log('hello snap');";
  std::string tar = BuildSnapTar(kSourceOnlyManifest, bundle, "dist/bundle.js");

  // Independent reference: base64(sha256(bundle)).
  EXPECT_EQ(Checksum(tar, bundle, kSourceOnlyManifest),
            base::Base64Encode(crypto::hash::Sha256(bundle)));
}

TEST(SnapInstallerChecksumCalculatorTest, MatchesTestHelperShasum) {
  // The Phase 4 installer tests rely on ComputeSnapBundleShasum producing a
  // shasum the calculator accepts; pin that equivalence here.
  const std::string bundle = "abc";
  std::string tar = BuildSnapTar(kSourceOnlyManifest, bundle, "dist/bundle.js");
  EXPECT_EQ(Checksum(tar, bundle, kSourceOnlyManifest),
            ComputeSnapBundleShasum(bundle));
}

TEST(SnapInstallerChecksumCalculatorTest, DeterministicAndInputSensitive) {
  std::string tar = BuildSnapTar(kSourceOnlyManifest, "bundle-a", "dist/bundle.js");
  std::string a = Checksum(tar, "bundle-a", kSourceOnlyManifest);
  std::string b = Checksum(tar, "bundle-b", kSourceOnlyManifest);
  EXPECT_NE(a, b);
  EXPECT_EQ(a, Checksum(tar, "bundle-a", kSourceOnlyManifest));
}

TEST(SnapInstallerChecksumCalculatorTest, IconAppendedToChecksum) {
  const std::string bundle = "BUNDLE";
  const std::string icon = "<svg></svg>";
  static constexpr char kManifest[] =
      R"({"source": {"shasum": "x",
          "location": {"npm": {"filePath": "dist/bundle.js",
                               "iconPath": "images/icon.svg"}}}})";
  std::string tar = BuildUstarTar({
      {"package/snap.manifest.json", kManifest},
      {"package/dist/bundle.js", bundle},
      {"package/images/icon.svg", icon},
  });

  // Canonical concatenation for an icon snap is bundle || icon.
  EXPECT_EQ(Checksum(tar, bundle, kManifest),
            base::Base64Encode(crypto::hash::Sha256(bundle + icon)));
}

TEST(SnapInstallerChecksumCalculatorTest, AuxFilesAppendedInSortedOrder) {
  const std::string bundle = "BUNDLE";
  // "files" is listed out of order; the checksum appends them sorted by path.
  static constexpr char kManifest[] =
      R"({"source": {"shasum": "x", "files": ["b.txt", "a.txt"],
          "location": {"npm": {"filePath": "dist/bundle.js"}}}})";
  std::string tar = BuildUstarTar({
      {"package/snap.manifest.json", kManifest},
      {"package/dist/bundle.js", bundle},
      {"package/a.txt", "AAA"},
      {"package/b.txt", "BBB"},
  });

  EXPECT_EQ(Checksum(tar, bundle, kManifest),
            base::Base64Encode(crypto::hash::Sha256(bundle + std::string("AAA") +
                                                    std::string("BBB"))));
}

}  // namespace brave_wallet

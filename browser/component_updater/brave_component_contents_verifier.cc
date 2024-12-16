// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/component_updater/brave_component_contents_verifier.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "brave/components/brave_component_updater/browser/component_contents_verifier.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)

#include "base/command_line.h"
#include "base/containers/span_reader.h"
#include "crypto/sha2.h"
#include "extensions/browser/content_hash_tree.h"
#include "extensions/browser/verified_contents.h"

namespace {

constexpr uint8_t kComponentContentsVerifierPublicKey[] = {
    0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
    0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xc3, 0x8a, 0x2d,
    0x68, 0x10, 0xb3, 0x52, 0xf1, 0xcc, 0xae, 0xd7, 0x70, 0xb7, 0xc9, 0xc7,
    0xe3, 0x1c, 0x91, 0x4c, 0x1a, 0x73, 0xf4, 0xbd, 0xb9, 0x54, 0x01, 0xb2,
    0xd0, 0x03, 0xfa, 0x23, 0x43, 0xb9, 0x52, 0xf4, 0xc8, 0x27, 0xbf, 0xed,
    0x6c, 0x9b, 0xba, 0x26, 0xa6, 0xe9, 0x43, 0x0f, 0x71, 0xf6, 0xa2, 0x2c,
    0x1e, 0xf9, 0xb3, 0xfc, 0xec, 0xfb, 0x12, 0xe1, 0xea, 0xbb, 0x71, 0x6d,
    0x80, 0x2b, 0x26, 0x5f, 0xfb, 0x20, 0x91, 0x26, 0x5a, 0x6d, 0xdd, 0x0e,
    0xd8, 0x88, 0xc2, 0x11, 0x7e, 0xf7, 0x5d, 0xfc, 0xd6, 0x99, 0x50, 0x9b,
    0x70, 0xe9, 0xa7, 0xcf, 0x7f, 0x4c, 0xf0, 0x80, 0x2d, 0x8d, 0x97, 0x9f,
    0xfc, 0x5c, 0x56, 0xc3, 0xff, 0x37, 0xf8, 0x3a, 0x44, 0x3a, 0x44, 0x3c,
    0xbe, 0x96, 0xf9, 0x14, 0x44, 0xb1, 0x8e, 0x08, 0xc2, 0x55, 0x49, 0xf1,
    0xe4, 0x3a, 0xa6, 0x63, 0x23, 0x26, 0x00, 0xa4, 0x8b, 0xdd, 0xaf, 0x45,
    0xb0, 0xf8, 0xb8, 0x24, 0x80, 0x85, 0x85, 0x95, 0x2e, 0xc5, 0xd4, 0xf1,
    0xdc, 0xfb, 0xeb, 0x95, 0xb6, 0x13, 0x3a, 0x46, 0x9d, 0x49, 0xb1, 0x15,
    0x72, 0xfd, 0x85, 0xc6, 0x0b, 0x7e, 0xe2, 0x97, 0xf1, 0x66, 0xa6, 0x0b,
    0x67, 0xe8, 0x72, 0xb9, 0xf1, 0x3b, 0x30, 0x1a, 0x77, 0x6d, 0xf3, 0xc4,
    0x99, 0xc0, 0x86, 0xb2, 0x4e, 0x1d, 0xde, 0x34, 0x86, 0x15, 0x41, 0x68,
    0x1e, 0x94, 0x48, 0x0a, 0x54, 0x05, 0x52, 0x01, 0x04, 0xd2, 0xef, 0x57,
    0x89, 0x47, 0xd3, 0xae, 0xd9, 0xc1, 0xf5, 0xa9, 0xbf, 0x60, 0x96, 0x4b,
    0xa0, 0x12, 0x9c, 0xf3, 0x00, 0xe6, 0x32, 0x40, 0xc7, 0x6e, 0x29, 0xa8,
    0x81, 0xfd, 0x2f, 0x1e, 0x92, 0x4a, 0x5e, 0xed, 0xef, 0x13, 0x9f, 0xed,
    0x88, 0x77, 0x2c, 0x84, 0xdd, 0x00, 0x87, 0x03, 0x49, 0x09, 0xb7, 0x4b,
    0xc7, 0x02, 0x03, 0x01, 0x00, 0x01};

constexpr char kVerifiedContentsPath[] = "_metadata/verified_contents.json";
constexpr char kBraveVerifiedContentsPath[] =
    "brave_metadata/verified_contents.json";

std::string GetRootHashForContent(base::span<const uint8_t> contents,
                                  size_t block_size) {
  CHECK(block_size % crypto::kSHA256Length == 0);

  std::vector<std::string> hashes;
  // Even when the contents is empty, we want to output at least one hash
  // block (the hash of the empty string).
  base::SpanReader reader(contents);
  do {
    const auto chunk = reader.Read(std::min(block_size, reader.remaining()));
    const auto hash = crypto::SHA256Hash(chunk.value());
    hashes.emplace_back(hash.begin(), hash.end());
  } while (reader.remaining() > 0);

  return extensions::ComputeTreeHashRoot(hashes,
                                         block_size / crypto::kSHA256Length);
}

class ExtensionsTreeHashContentsVerifier
    : public component_updater::ContentsVerifier {
 public:
  explicit ExtensionsTreeHashContentsVerifier(
      const base::FilePath& component_root) {
    if (base::PathExists(
            component_root.AppendASCII(kBraveVerifiedContentsPath))) {
      verified_contents_ = extensions::VerifiedContents::CreateFromFile(
          kComponentContentsVerifierPublicKey,
          component_root.AppendASCII(kBraveVerifiedContentsPath));
    } else {
      verified_contents_ = extensions::VerifiedContents::CreateFromFile(
          kComponentContentsVerifierPublicKey,
          component_root.AppendASCII(kVerifiedContentsPath));
    }
    if (verified_contents_ &&
        verified_contents_->block_size() % crypto::kSHA256Length != 0) {
      // Unsupported block size.
      verified_contents_.reset();
    }
  }

  bool VerifyContents(const base::FilePath& relative_path,
                      base::span<const uint8_t> contents) override {
    if (!verified_contents_ ||
        !verified_contents_->HasTreeHashRoot(relative_path)) {
      return false;
    }

    const auto root_hash =
        GetRootHashForContent(contents, verified_contents_->block_size());
    return verified_contents_->TreeHashRootEquals(relative_path, root_hash);
  }

 private:
  std::unique_ptr<extensions::VerifiedContents> verified_contents_;
};

bool ShouldBypassSignature() {
  static const bool kBypass =
      !base::FeatureList::IsEnabled(
          component_updater::kComponentContentsVerifier) ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          component_updater::kBypassComponentContentsVerifier);
  return kBypass;
}

std::unique_ptr<component_updater::ContentsVerifier>
CreateExtensionsTreeHashContentsVerifier(const base::FilePath& component_root) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!ShouldBypassSignature()) {
    return std::make_unique<ExtensionsTreeHashContentsVerifier>(component_root);
  }
#endif
  // if there is no extensions enabled then we expect that on these platforms
  // the component files are protected by the OS.
  return nullptr;
}

}  // namespace
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

namespace component_updater {

BASE_FEATURE(kComponentContentsVerifier,
             "ComponentContentsVerifier",
             base::FEATURE_DISABLED_BY_DEFAULT);

void SetupComponentContentsVerifier() {
  auto factory = base::BindRepeating(CreateExtensionsTreeHashContentsVerifier);
  SetContentsVerifierFactory(std::move(factory));
}

}  // namespace component_updater

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

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/containers/span_reader.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "brave/components/brave_component_updater/browser/component_contents_verifier.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "crypto/sha2.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)

#include "extensions/browser/content_hash_tree.h"
#include "extensions/browser/verified_contents.h"

namespace {

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

class ExtensionsTreeHashContentChecker
    : public component_updater::ContentChecker {
 public:
  ExtensionsTreeHashContentChecker(size_t block_size,
                                   std::vector<std::string> possible_hashes)
      : block_size_(block_size), possible_hashes_(std::move(possible_hashes)) {
    CHECK(!possible_hashes_.empty());
  }

  ~ExtensionsTreeHashContentChecker() override = default;
  bool VerifyContents(base::span<const uint8_t> contents) const override {
    return base::Contains(possible_hashes_,
                          GetRootHashForContent(contents, block_size_));
  }

 private:
  const size_t block_size_ = 0;
  const std::vector<std::string> possible_hashes_;
};

class ExtensionsTreeHashContentsVerifier
    : public component_updater::ContentsVerifier {
 public:
  explicit ExtensionsTreeHashContentsVerifier(
      const base::FilePath& component_root) {
    if (base::PathExists(
            component_root.AppendASCII(kBraveVerifiedContentsPath))) {
      verified_contents_ = extensions::VerifiedContents::CreateFromFile(
          extensions::kBraveVerifiedContentsPublicKey,
          component_root.AppendASCII(kBraveVerifiedContentsPath));
    } else {
      verified_contents_ = extensions::VerifiedContents::CreateFromFile(
          extensions::kBraveVerifiedContentsPublicKey,
          component_root.AppendASCII(kVerifiedContentsPath));
    }
    if (verified_contents_ &&
        verified_contents_->block_size() % crypto::kSHA256Length != 0) {
      // Unsupported block size.
      verified_contents_.reset();
    }
  }

  bool IsValid() const override { return !!verified_contents_; }

  std::unique_ptr<component_updater::ContentChecker> CreateContentChecker(
      const base::FilePath& relative_path) const override {
    std::vector<std::string> hashes;
    if (!verified_contents_) {
      CHECK_IS_TEST();
    } else {
      hashes = verified_contents_->GetRootHashes(relative_path);
    }
    if (hashes.empty()) {
      // File is not signed.
      return nullptr;
    }
    return std::make_unique<ExtensionsTreeHashContentChecker>(
        verified_contents_->block_size(), std::move(hashes));
  }

 private:
  std::unique_ptr<const extensions::VerifiedContents> verified_contents_;
};

bool ShouldBypassSignature() {
  static const bool kBypass =
      !base::FeatureList::IsEnabled(
          brave_component_updater::kComponentContentsVerifier) ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          component_updater::kBypassComponentContentsVerifier);
  return kBypass;
}

}  // namespace
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

namespace component_updater {

namespace {
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

void SetupComponentContentsVerifier() {
  auto factory = base::BindRepeating(CreateExtensionsTreeHashContentsVerifier);
  SetContentsVerifierFactory(std::move(factory));
}

}  // namespace component_updater

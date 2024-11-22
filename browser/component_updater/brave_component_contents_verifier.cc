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
#include "base/functional/bind.h"
#include "crypto/secure_hash.h"
#include "crypto/sha2.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/content_hash_tree.h"
#include "extensions/browser/verified_contents.h"
#endif

namespace {

// Only proxies the file access.
class ComponentNoChecksContentsAccessorImpl
    : public brave_component_updater::ComponentContentsAccessor {
 public:
  explicit ComponentNoChecksContentsAccessorImpl(
      const base::FilePath& component_root)
      : component_root_(component_root) {}

  const base::FilePath& GetComponentRoot() const override {
    return component_root_;
  }

  bool IsComponentSignatureValid() const override { return true; }

  void IgnoreInvalidSignature(bool) override {}

  bool VerifyContents(const base::FilePath& relative_path,
                      base::span<const uint8_t> contents) override {
    return true;
  }

 protected:
  ~ComponentNoChecksContentsAccessorImpl() override = default;

  const base::FilePath component_root_;
};

}  // namespace

#if BUILDFLAG(ENABLE_EXTENSIONS)

namespace {

constexpr const uint8_t kComponentContentsVerifierPublicKey[] = {
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

constexpr const char kVerifiedContentsPath[] =
    "_metadata/verified_contents.json";

std::string GetRootHasheForContent(base::span<const uint8_t> contents,
                                   size_t block_size) {
  size_t offset = 0;
  std::vector<std::string> hashes;
  // Even when the contents is empty, we want to output at least one hash
  // block (the hash of the empty string).
  do {
    const size_t bytes_to_read = std::min(contents.size() - offset, block_size);
    std::unique_ptr<crypto::SecureHash> hash(
        crypto::SecureHash::Create(crypto::SecureHash::SHA256));
    hash->Update(contents.subspan(offset, bytes_to_read));

    std::string buffer;
    buffer.resize(crypto::kSHA256Length);
    hash->Finish(base::as_writable_byte_span(buffer));
    hashes.push_back(std::move(buffer));

    // If |contents| is empty, then we want to just exit here.
    if (bytes_to_read == 0) {
      break;
    }

    offset += bytes_to_read;
  } while (offset < contents.size());

  CHECK(block_size % crypto::kSHA256Length == 0);
  return extensions::ComputeTreeHashRoot(hashes,
                                         block_size / crypto::kSHA256Length);
}

// Proxies the file access and checks the verified_contents.json.
class ComponentContentsAccessorImpl
    : public ComponentNoChecksContentsAccessorImpl {
 public:
  explicit ComponentContentsAccessorImpl(const base::FilePath& component_root)
      : ComponentNoChecksContentsAccessorImpl(component_root) {
    verified_contents_ = extensions::VerifiedContents::CreateFromFile(
        base::make_span(kComponentContentsVerifierPublicKey),
        component_root.AppendASCII(kVerifiedContentsPath));
    if (verified_contents_->block_size() % crypto::kSHA256Length != 0) {
      // Unsupported block size.
      verified_contents_.reset();
    }
  }

  bool IsComponentSignatureValid() const override {
    return !!verified_contents_.get();
  }

  void IgnoreInvalidSignature(bool ignore) override {
    ignore_invalid_signature_ = ignore;
  }

  bool VerifyContents(const base::FilePath& relative_path,
                      base::span<const uint8_t> contents) override {
    if (!IsComponentSignatureValid()) {
      return ignore_invalid_signature_;
    }

    if (!verified_contents_->HasTreeHashRoot(relative_path)) {
      return true;
    }

    const auto root_hash =
        GetRootHasheForContent(contents, verified_contents_->block_size());
    if (!verified_contents_->TreeHashRootEquals(relative_path, root_hash)) {
      return false;
    }
    return true;
  }

 private:
  ~ComponentContentsAccessorImpl() override = default;

  std::unique_ptr<extensions::VerifiedContents> verified_contents_;
  bool ignore_invalid_signature_ = false;
};

}  // namespace

#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

namespace component_updater {

scoped_refptr<brave_component_updater::ComponentContentsAccessor>
CreateComponentContentsAccessor(bool with_verifier,
                                const base::FilePath& component_root) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (with_verifier) {
    return base::MakeRefCounted<ComponentContentsAccessorImpl>(component_root);
  }
#endif
  // if there is no extensions enabled then we expect that on these platforms
  // the component files are protected by the OS.
  return base::MakeRefCounted<ComponentNoChecksContentsAccessorImpl>(
      component_root);
}

void SetupComponentContentsVerifier() {
  auto factory = base::BindRepeating(CreateComponentContentsAccessor, true);
  brave_component_updater::ComponentContentsVerifier::Setup(std::move(factory));
}

}  // namespace component_updater

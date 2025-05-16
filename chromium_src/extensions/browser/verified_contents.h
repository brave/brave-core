/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_VERIFIED_CONTENTS_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_VERIFIED_CONTENTS_H_

#include "base/memory/raw_span.h"

#define VerifiedContents VerifiedContents_ChromiumImpl

#include "src/extensions/browser/verified_contents.h"  // IWYU pragma: export

#undef VerifiedContents

namespace extensions {

// Wraps VerifiedContents_ChromiumImpl instance and repeates the its public
// interface.
class VerifiedContents {
 public:
  ~VerifiedContents();

  static std::unique_ptr<VerifiedContents> CreateFromFile(
      base::span<const uint8_t> public_key,
      const base::FilePath& path);

  static std::unique_ptr<VerifiedContents> Create(
      base::span<const uint8_t> public_key,
      std::string_view contents);

  int block_size() const;
  const ExtensionId& extension_id() const;
  const base::Version& version() const;

  bool HasTreeHashRoot(const base::FilePath& relative_path) const;

  bool TreeHashRootEquals(const base::FilePath& relative_path,
                          const std::string& expected) const;

  bool TreeHashRootEqualsForCanonicalPath(
      const CanonicalRelativePath& canonical_relative_path,
      const std::string& expected) const;
  bool valid_signature();

 private:
  explicit VerifiedContents(
      std::unique_ptr<VerifiedContents_ChromiumImpl> verified_contents);

  std::unique_ptr<VerifiedContents_ChromiumImpl> verified_contents_;
};

}  // namespace extensions

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_VERIFIED_CONTENTS_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/browser/verified_contents.h"

#include "base/memory/ptr_util.h"
#include "extensions/common/extension.h"

/*
  It's a trick to reimplement VerifiedContents::Create(...) function without
  creating the git patch.

  Original code:

  ```
  std::unique_ptr<VerifiedContents> VerifiedContents::Create(
    base::span<const uint8_t> public_key,
    std::string_view contents) {
  // Note: VerifiedContents constructor is private.
  auto verified_contents = base::WrapUnique(new VerifiedContents(public_key));
  std::string payload;
  if (!verified_contents->GetPayload(contents, &payload))
  ```

  Modified:

  ```
  std::unique_ptr<VerifiedContents> VerifiedContents::Create(
    base::span<const uint8_t> public_key,
    std::string_view contents) {
  // Note: VerifiedContents constructor is private.
  auto verified_contents = base::WrapUnique(
      Create_ChromiumImpl(new VerifiedContents(public_key), contents)
          .release());
   if (verified_contents) {
     // OK, |contents| signed by |public_key|. Use it.
     return verified_contents;
   }
   // Trying Brave's key.
   return Create_ChromiumImpl(
      new VerifiedContents(kBraveVerifiedContentsPublicKey), contents);
  }

  // Checks whether the public key contained in |vc| is suitable for |contents|.
  std ::unique_ptr<VerifiedContents> VerifiedContents ::Create_ChromiumImpl(
    VerifiedContents* vc,
    std ::string_view contents) {
  std ::unique_ptr<VerifiedContents> verified_contents(vc);
  std::string payload;
  if (!verified_contents->GetPayload(contents, &payload))

  ```
*/

#define WrapUnique(...)                                                    \
  WrapUnique(Create_ChromiumImpl(__VA_ARGS__, contents).release());        \
  if (verified_contents) {                                                 \
    return verified_contents;                                              \
  }                                                                        \
  return Create_ChromiumImpl(                                              \
      new VerifiedContents(kBraveVerifiedContentsPublicKey), contents);    \
  }                                                                        \
                                                                           \
  std::unique_ptr<VerifiedContents> VerifiedContents::Create_ChromiumImpl( \
      VerifiedContents* vc, std::string_view contents) {                   \
    std::unique_ptr<VerifiedContents> verified_contents(vc)

#include <extensions/browser/verified_contents.cc>

namespace extensions {

std::vector<std::string> VerifiedContents::GetRootHashes(
    const base::FilePath& relative_path) const {
  std::pair<RootHashes::const_iterator, RootHashes::const_iterator> hashes =
      root_hashes_.equal_range(
          content_verifier_utils::CanonicalizeRelativePath(relative_path));

  std::vector<std::string> root_hashes;
  for (auto iter = hashes.first; iter != hashes.second; ++iter) {
    root_hashes.push_back(iter->second);
  }
  return root_hashes;
}

}  // namespace extensions

#undef WrapUnique

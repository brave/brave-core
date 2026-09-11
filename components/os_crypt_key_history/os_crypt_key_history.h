/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OS_CRYPT_KEY_HISTORY_OS_CRYPT_KEY_HISTORY_H_
#define BRAVE_COMPONENTS_OS_CRYPT_KEY_HISTORY_OS_CRYPT_KEY_HISTORY_H_

#include <stddef.h>

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/time/time.h"

namespace brave {

// One encryption key that was seen decrypting this profile's data.
struct OSCryptKeyRecord {
  // The key exactly as `Local State` stores it: base64 of the platform-wrapped
  // key. Never plaintext key material.
  std::string wrapped_key;
  // When this key was first recorded.
  base::Time first_seen;
  // The last time this key was seen working.
  base::Time last_verified;
};

// A short history of encryption keys that have been proven to decrypt this
// profile's data, kept in a file of its own so that it survives `Local State`
// being replaced. See https://github.com/brave/brave-browser/issues/40375 and
// README.md.
//
// The store holds no cryptography and no platform code. It is told which keys
// are known good, and it is handed a predicate for deciding whether a stored
// key is really the same key, because wrapping the same key twice produces
// different bytes each time.
class OSCryptKeyHistory {
 public:
  // How many keys are kept per provider. Enough to survive a key being
  // replaced and then replaced again before anyone notices, without keeping
  // old key material around indefinitely.
  static constexpr size_t kMaxRecordsPerProvider = 3;

  enum class LoadResult {
    // The file was read, and any records it held are now in memory.
    kLoaded,
    // There is no file yet. The history is empty and it is safe to write one.
    kNoFile,
    // A file is there but could not be read, parsed, or understood. The
    // history is empty, and overwriting the file would discard records we
    // failed to read.
    kUnreadable,
  };

  // Answers whether a stored `wrapped_key` holds the same key material as the
  // key being recorded. Supplied by the caller because unwrapping is platform
  // specific, and because comparing wrapped bytes gives the wrong answer:
  // re-wrapping a key produces different bytes for the same key.
  using SameKeyPredicate =
      base::RepeatingCallback<bool(std::string_view wrapped_key)>;

  explicit OSCryptKeyHistory(base::FilePath path);
  OSCryptKeyHistory(const OSCryptKeyHistory&) = delete;
  OSCryptKeyHistory& operator=(const OSCryptKeyHistory&) = delete;
  ~OSCryptKeyHistory();

  // Reads the history from disk, replacing anything held in memory. Blocking.
  LoadResult Load();

  // Writes the history to disk, atomically. Blocking. Callers that got
  // `kUnreadable` from `Load()` should not call this: it would replace records
  // that could not be read with an empty history.
  bool Save() const;

  // Records that `wrapped_key` was seen decrypting real user data at `now`.
  //
  // A key that is already held is left as it is, apart from its
  // `last_verified` time: the stored bytes are just as good, and rewriting
  // them buys nothing. A key that is not held becomes the newest record, and
  // the keys already there are kept, because a key that stops working is
  // exactly the one that may need handing back.
  void RecordVerifiedKey(std::string_view provider,
                         std::string_view wrapped_key,
                         base::Time now,
                         const SameKeyPredicate& is_same_key);

  // Records held for `provider`, newest first. Empty if there are none.
  const std::vector<OSCryptKeyRecord>& GetRecords(
      std::string_view provider) const;

 private:
  const base::FilePath path_;
  std::map<std::string, std::vector<OSCryptKeyRecord>, std::less<>> providers_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_OS_CRYPT_KEY_HISTORY_OS_CRYPT_KEY_HISTORY_H_

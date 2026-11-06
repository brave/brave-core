// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_PASSAGES_UTIL_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_PASSAGES_UTIL_H_

#include <optional>

#include "base/containers/span.h"
#include "brave/components/history_embeddings/proto/history_embeddings.pb.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace brave_history_embeddings {

// Compresses and encrypts `passages_value` to a bytestring blob suitable for
// storage in the database. Returns an empty vector on error.
std::vector<uint8_t> PassagesProtoToBlob(
    const proto::PassagesValue& passages_value,
    const os_crypt_async::Encryptor& encryptor);

// Decrypts and then decompresses `passages_blob` suitable for storage in the
// database to the original proto. Note that this can fail for invalid inputs,
// and therefore can return nullopt.
std::optional<proto::PassagesValue> PassagesBlobToProto(
    base::span<const uint8_t> passages_blob,
    const os_crypt_async::Encryptor& encryptor);

}  // namespace brave_history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_PASSAGES_UTIL_H_

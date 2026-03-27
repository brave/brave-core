// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_FILE_UTIL_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_FILE_UTIL_H_

#include <optional>

#include "base/files/file_path.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace local_ai {

// Reads a file directly into BigBuffer's internal storage. For
// files >64KB BigBuffer uses shared memory — the file data goes
// straight into that allocation, avoiding a separate heap copy.
std::optional<mojo_base::BigBuffer> ReadFileToBigBuffer(
    const base::FilePath& path);

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_FILE_UTIL_H_

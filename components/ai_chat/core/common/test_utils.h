/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_UTILS_H_

#include <optional>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

std::vector<mojom::UploadedFilePtr> CreateSampleUploadedFiles(
    size_t number,
    std::optional<mojom::UploadedFileType> type = std::nullopt);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_UTILS_H_

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_TEST_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_TEST_UTILS_H_

#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/location.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

// Returns the history with a modified server reply in edits.
std::vector<mojom::ConversationTurnPtr> GetHistoryWithModifiedReply();

// Helper functions to verify content blocks in tests
void VerifyTextBlock(const base::Location& location,
                     const mojom::ContentBlockPtr& block,
                     std::string_view expected_text);

void VerifyImageBlock(const base::Location& location,
                      const mojom::ContentBlockPtr& block,
                      const GURL& expected_url);

void VerifyFileBlock(const base::Location& location,
                     const mojom::ContentBlockPtr& block,
                     const GURL& expected_url,
                     std::string_view expected_filename);

void VerifyPageTextBlock(const base::Location& location,
                         const mojom::ContentBlockPtr& block,
                         std::string_view expected_text);

void VerifyPageExcerptBlock(const base::Location& location,
                            const mojom::ContentBlockPtr& block,
                            std::string_view expected_text);

// Helper to build expected memory for testing
base::flat_map<std::string, mojom::MemoryValuePtr> BuildExpectedMemory(
    const base::flat_map<std::string, std::string>& string_values,
    const base::flat_map<std::string, std::vector<std::string>>& list_values);

void VerifyMemoryBlock(
    const base::Location& location,
    const mojom::ContentBlockPtr& block,
    const base::flat_map<std::string, mojom::MemoryValuePtr>& expected_memory);

void VerifyVideoTranscriptBlock(const base::Location& location,
                                const mojom::ContentBlockPtr& block,
                                std::string_view expected_text);

void VerifyRequestTitleBlock(const base::Location& location,
                             const mojom::ContentBlockPtr& block,
                             std::string_view expected_text);

void VerifyChangeToneBlock(const base::Location& location,
                           const mojom::ContentBlockPtr& block,
                           std::string_view expected_text,
                           std::string_view expected_tone);

void VerifySimpleRequestBlock(const base::Location& location,
                              const mojom::ContentBlockPtr& block,
                              mojom::SimpleRequestType expected_type);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_TEST_UTILS_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_CONVERSIONS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_CONVERSIONS_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

namespace sync_pb {
class AIChatConversationSpecifics;
class EntitySpecifics;
}  // namespace sync_pb

namespace syncer {
struct EntityData;
}  // namespace syncer

namespace ai_chat {

// Converts a Conversation and its full archive data into sync specifics.
sync_pb::AIChatConversationSpecifics ConversationToSpecifics(
    const mojom::Conversation& conversation,
    const mojom::ConversationArchive& archive);

// Creates EntityData from specifics, suitable for passing to
// change_processor()->Put().
std::unique_ptr<syncer::EntityData> CreateEntityDataFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics);

// Returns the storage key from specifics (the conversation UUID).
std::string GetStorageKeyFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics);

// Returns the client tag from specifics (the conversation UUID).
std::string GetClientTagFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics);

// Extracts the storage key from an EntitySpecifics wrapper.
std::string GetStorageKeyFromEntitySpecifics(
    const sync_pb::EntitySpecifics& specifics);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_CONVERSIONS_H_

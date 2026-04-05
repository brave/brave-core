/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/history/core/browser/sync/brave_history_data_type_controller.h"
#include "brave/components/history/core/browser/sync/brave_history_delete_directives_data_type_controller.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "components/sync/service/data_type_controller.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_AI_CHAT)
#define BRAVE_BUILD_SYNC_CONTROLLERS                                       \
  if (ai_chat_service_.value() &&                                          \
      ai_chat::features::IsBraveSyncAIChatEnabled()) {                     \
    auto full_delegate =                                                   \
        ai_chat_service_.value()->CreateSyncControllerDelegate();          \
    auto transport_delegate =                                              \
        ai_chat_service_.value()->CreateSyncControllerDelegate();          \
    if (full_delegate && transport_delegate) {                             \
      controllers.push_back(std::make_unique<syncer::DataTypeController>(  \
          syncer::AI_CHAT_CONVERSATION,                                    \
          /*delegate_for_full_sync_mode=*/std::move(full_delegate),        \
          /*delegate_for_transport_mode=*/std::move(transport_delegate))); \
    }                                                                      \
  }
#else
#define BRAVE_BUILD_SYNC_CONTROLLERS
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#define HistoryDeleteDirectivesDataTypeController \
  BraveHistoryDeleteDirectivesDataTypeController

#define HistoryDataTypeController BraveHistoryDataTypeController

#include <components/browser_sync/common_controller_builder.cc>

#undef HistoryDataTypeController
#undef HistoryDeleteDirectivesDataTypeController
#undef BRAVE_BUILD_SYNC_CONTROLLERS

// Setter implementation — defined after the include so the class is complete.
namespace browser_sync {
void CommonControllerBuilder::SetAIChatService(
    ai_chat::AIChatService* ai_chat_service) {
  ai_chat_service_.Set(ai_chat_service);
}
}  // namespace browser_sync

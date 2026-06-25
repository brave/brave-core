/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sync/service/data_type_controller.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "base/functional/bind.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_data_type_controller.h"
#include "brave/components/ai_chat/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

namespace syncer {
class SyncService;
}  // namespace syncer

// Returns the controllers of Brave's own sync data types, which
// SyncServiceFactory registers along with upstream's.
syncer::DataTypeController::TypeVector CreateBraveControllers(
    Profile* profile,
    syncer::SyncService* sync_service) {
  syncer::DataTypeController::TypeVector controllers;
#if BUILDFLAG(ENABLE_AI_CHAT)
  // The controller reads AI Chat prefs, which are registered only while AI
  // Chat is enabled. It gets a getter rather than AIChatService, because
  // building AIChatService here would re-enter SyncServiceFactory; see
  // AIChatDataTypeController.
  if (ai_chat::features::IsAIChatEnabled() &&
      ai_chat::features::IsBraveSyncAIChatEnabled()) {
    controllers.push_back(std::make_unique<ai_chat::AIChatDataTypeController>(
        sync_service, profile->GetPrefs(),
        base::BindRepeating(
            &ai_chat::AIChatServiceFactory::GetForBrowserContext, profile)));
  }
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
  return controllers;
}

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "components/sync/service/data_type_controller.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "base/functional/bind.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_data_type_controller.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

namespace syncer {
class SyncService;
}  // namespace syncer

// Returns the controllers of Brave's own sync data types, which
// SyncServiceFactory registers along with upstream's.
syncer::DataTypeController::TypeVector CreateBraveControllers(
    ProfileIOS* profile,
    syncer::SyncService* sync_service) {
  syncer::DataTypeController::TypeVector controllers;
#if BUILDFLAG(ENABLE_AI_CHAT)
  // The controller reads AI Chat prefs, which are registered only while AI
  // Chat is enabled. It gets a getter rather than AIChatService, which it runs
  // only once the SyncService is built; see AIChatDataTypeController.
  if (ai_chat::features::IsAIChatEnabled() &&
      ai_chat::features::IsBraveSyncAIChatEnabled()) {
    controllers.push_back(std::make_unique<ai_chat::AIChatDataTypeController>(
        sync_service, profile->GetPrefs(),
        base::BindRepeating(&ai_chat::AIChatServiceFactory::GetForProfile,
                            profile)));
  }
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
  return controllers;
}

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync/brave_sync_service_impl_delegate.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "ios/chrome/browser/history/model/history_service_factory.h"
#include "ios/chrome/browser/sync/model/device_info_sync_service_factory.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#define BRAVE_BUILD_SERVICE_INSTANCE_FOR                        \
  std::make_unique<syncer::BraveSyncServiceImpl>(               \
      std::move(init_params),                                   \
      std::make_unique<syncer::BraveSyncServiceImplDelegate>(   \
          DeviceInfoSyncServiceFactory::GetForProfile(profile), \
          ios::HistoryServiceFactory::GetForProfile(            \
              profile, ServiceAccessType::IMPLICIT_ACCESS)));

#if BUILDFLAG(ENABLE_AI_CHAT)
#define BRAVE_COMMON_CONTROLLER_BUILDER_SET_SERVICES \
  builder.SetAIChatService(                          \
      ai_chat::AIChatServiceFactory::GetForProfile(profile));
#else
#define BRAVE_COMMON_CONTROLLER_BUILDER_SET_SERVICES
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#include <ios/chrome/browser/sync/model/sync_service_factory.mm>

#undef BRAVE_COMMON_CONTROLLER_BUILDER_SET_SERVICES
#undef BRAVE_BUILD_SERVICE_INSTANCE_FOR

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSER_SYNC_COMMON_CONTROLLER_BUILDER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSER_SYNC_COMMON_CONTROLLER_BUILDER_H_

namespace ai_chat {
class AIChatService;
}  // namespace ai_chat

#define SetSkillsService(...)                                \
  SetAIChatService(ai_chat::AIChatService* ai_chat_service); \
  void SetSkillsService(__VA_ARGS__)

#define skills_service_ \
  skills_service_;      \
  SafeOptional<raw_ptr<ai_chat::AIChatService>> ai_chat_service_

#include <components/browser_sync/common_controller_builder.h>  // IWYU pragma: export

#undef skills_service_
#undef SetSkillsService

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSER_SYNC_COMMON_CONTROLLER_BUILDER_H_

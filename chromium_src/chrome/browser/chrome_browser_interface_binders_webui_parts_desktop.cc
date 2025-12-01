/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_origin/common/mojom/brave_origin_settings.mojom.h"
#include "brave/components/commands/common/commands.mojom.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/brave_account_row.mojom.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/ollama.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/settings_helper.mojom.h"
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/core/mojom/containers.mojom.h"
#endif

// clang-format off
#if BUILDFLAG(ENABLE_AI_CHAT)
#define AI_CHAT_HANDLERS                       \
  .Add<ai_chat::mojom::AIChatSettingsHelper>() \
  .Add<ai_chat::mojom::CustomizationSettingsHandler>() \
  .Add<ai_chat::mojom::OllamaService>()
#else
#define AI_CHAT_HANDLERS
#endif
// clang-format on

#if BUILDFLAG(ENABLE_CONTAINERS)
#define CONTAINERS_HANDLERS .Add<containers::mojom::ContainersSettingsHandler>()
#else
#define CONTAINERS_HANDLERS
#endif

// clang-format off
#if !BUILDFLAG(IS_ANDROID)
#define DESKTOP_HANDLERS                                  \
  .Add<brave_account::mojom::Authentication>()            \
  .Add<brave_account::mojom::RowHandlerFactory>()         \
  .Add<brave_origin::mojom::BraveOriginSettingsHandler>() \
  AI_CHAT_HANDLERS
#else
#define DESKTOP_HANDLERS
#endif
// clang-format on

// clang-format off
#define BRAVE_POPULATE_CHROME_WEBUI_FRAME_INTERFACE_BROKERS_TRUSTED_PARTS_DESKTOP \
  .Add<commands::mojom::CommandsService>()                                    \
  .Add<email_aliases::mojom::EmailAliasesService>()                           \
  CONTAINERS_HANDLERS                                                         \
  DESKTOP_HANDLERS
// clang-format on

#include <chrome/browser/chrome_browser_interface_binders_webui_parts_desktop.cc>

#undef BRAVE_POPULATE_CHROME_WEBUI_FRAME_INTERFACE_BROKERS_TRUSTED_PARTS_DESKTOP
#undef DESKTOP_HANDLERS
#undef CONTAINERS_HANDLERS
#undef AI_CHAT_HANDLERS

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_UI_H_

#include "brave/components/containers/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace ai_chat::mojom {
class AIChatSettingsHelper;
class CustomizationSettingsHandler;
}  // namespace ai_chat::mojom

namespace brave_account::mojom {
class BraveAccountSettingsHandler;
}

namespace brave_origin::mojom {
class BraveOriginSettingsHandler;
}

namespace commands::mojom {
class CommandsService;
}

namespace email_aliases::mojom {
class EmailAliasesService;
}

#if BUILDFLAG(ENABLE_CONTAINERS)
namespace containers::mojom {
class ContainersSettingsHandler;
}

#define BIND_CONTAINERS_INTERFACES                                        \
  virtual void BindInterface(                                             \
      mojo::PendingReceiver<containers::mojom::ContainersSettingsHandler> \
          pending_receiver) {}
#else
#define BIND_CONTAINERS_INTERFACES
#endif

#define AddSettingsPageUIHandler(...)                                          \
  AddSettingsPageUIHandler(__VA_ARGS__);                                       \
                                                                               \
 public:                                                                       \
  virtual void BindInterface(                                                  \
      mojo::PendingReceiver<commands::mojom::CommandsService>                  \
          pending_receiver) {}                                                 \
  virtual void BindInterface(                                                  \
      mojo::PendingReceiver<ai_chat::mojom::AIChatSettingsHelper>              \
          pending_receiver) {}                                                 \
  virtual void BindInterface(                                                  \
      mojo::PendingReceiver<ai_chat::mojom::CustomizationSettingsHandler>      \
          pending_receiver) {}                                                 \
  virtual void BindInterface(                                                  \
      mojo::PendingReceiver<brave_account::mojom::BraveAccountSettingsHandler> \
          pending_receiver) {}                                                 \
  virtual void BindInterface(                                                  \
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesService>         \
          pending_receiver) {}                                                 \
  virtual void BindInterface(                                                  \
      mojo::PendingReceiver<brave_origin::mojom::BraveOriginSettingsHandler>   \
          pending_receiver) {}                                                 \
  BIND_CONTAINERS_INTERFACES                                                   \
                                                                               \
 private:                                                                      \
  void UnUsed()

#include <chrome/browser/ui/webui/settings/settings_ui.h>  // IWYU pragma: export

#undef AddSettingsPageUIHandler
#undef BIND_CONTAINERS_INTERFACES

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_UI_H_

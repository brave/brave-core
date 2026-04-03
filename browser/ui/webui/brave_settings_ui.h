/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SETTINGS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SETTINGS_UI_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/brave_account_row.mojom.h"
#include "brave/components/brave_origin/mojom/brave_origin_settings.mojom.h"
#include "brave/components/commands/common/commands.mojom.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "chrome/browser/ui/webui/settings/settings_ui.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "ui/webui/resources/cr_components/color_change_listener/color_change_listener.mojom.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/ollama.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/settings_helper.mojom.h"
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/core/mojom/containers.mojom.h"
#endif

class BraveSettingsUI;

namespace content {
class WebUIDataSource;
}

class Profile;

class BraveSettingsUIConfig
    : public content::DefaultWebUIConfig<BraveSettingsUI> {
 public:
  BraveSettingsUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme,
                           chrome::kChromeUISettingsHost) {}
};

class BraveSettingsUI : public settings::SettingsUI {
 public:
  explicit BraveSettingsUI(content::WebUI* web_ui);
  BraveSettingsUI(const BraveSettingsUI&) = delete;
  BraveSettingsUI& operator=(const BraveSettingsUI&) = delete;
  ~BraveSettingsUI() override;

  static void AddResources(content::WebUIDataSource* html_source,
                           Profile* profile);
  static bool& ShouldExposeElementsForTesting();

  void BindInterface(
      mojo::PendingReceiver<commands::mojom::CommandsService> pending_receiver);
#if BUILDFLAG(ENABLE_AI_CHAT)
  void BindInterface(mojo::PendingReceiver<ai_chat::mojom::AIChatSettingsHelper>
                         pending_receiver);
  void BindInterface(
      mojo::PendingReceiver<ai_chat::mojom::CustomizationSettingsHandler>
          pending_receiver);
  void BindInterface(
      mojo::PendingReceiver<ai_chat::mojom::OllamaService> pending_receiver);
#endif
  void BindInterface(mojo::PendingReceiver<brave_account::mojom::Authentication>
                         pending_receiver);
  void BindInterface(
      mojo::PendingReceiver<brave_account::mojom::RowHandler> pending_receiver);

#if BUILDFLAG(ENABLE_CONTAINERS)
  void BindInterface(
      mojo::PendingReceiver<containers::mojom::ContainersSettingsHandler>
          pending_receiver);
#endif

  void BindInterface(
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesService>
          pending_receiver);

  void BindInterface(
      mojo::PendingReceiver<brave_origin::mojom::BraveOriginSettingsHandler>
          pending_receiver);

 private:
  // Makes a self-owned receiver and tracks it for cleanup. This guarantees the
  // handler is destroyed before its dependencies, preventing dangling pointers
  // during teardown.
  template <typename Interface, typename Impl>
  void MakeOwnedReceiver(std::unique_ptr<Impl> impl,
                         mojo::PendingReceiver<Interface> receiver) {
    auto ref =
        mojo::MakeSelfOwnedReceiver(std::move(impl), std::move(receiver));

    receiver_cleanup_runners_.emplace_back(base::BindOnce(
        [](mojo::SelfOwnedReceiverRef<Interface> r) {
          if (r) {
            r->Close();
          }
        },
        std::move(ref)));
  }

  // Ensure the mojo receivers are destroyed when the UI is destroyed.
  std::vector<base::ScopedClosureRunner> receiver_cleanup_runners_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SETTINGS_UI_H_

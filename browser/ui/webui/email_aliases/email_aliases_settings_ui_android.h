/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_SETTINGS_UI_ANDROID_H_
#define BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_SETTINGS_UI_ANDROID_H_

#include <string>

#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class EmailAliasesSettingsUIAndroid
    : public content::WebUIController,
      public brave_account::mojom::DialogController {
 public:
  explicit EmailAliasesSettingsUIAndroid(content::WebUI* web_ui);
  EmailAliasesSettingsUIAndroid(const EmailAliasesSettingsUIAndroid&) = delete;
  EmailAliasesSettingsUIAndroid& operator=(
      const EmailAliasesSettingsUIAndroid&) = delete;
  ~EmailAliasesSettingsUIAndroid() override;

  void BindInterface(
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesService>
          receiver);
  void BindInterface(
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesMetrics>
          receiver);
  void BindInterface(
      mojo::PendingReceiver<brave_account::mojom::Authentication> receiver);
  void BindInterface(
      mojo::PendingReceiver<brave_account::mojom::DialogController> receiver);

 private:
  void OpenDialog(const std::string& initiating_service_name,
                  brave_account::mojom::DialogMode dialog_mode) override;
  void CloseDialog() override;
  void GetDialogMode(GetDialogModeCallback callback) override;

  mojo::Receiver<brave_account::mojom::DialogController>
      dialog_controller_receiver_{this};

 public:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class EmailAliasesSettingsUIAndroidConfig
    : public content::DefaultWebUIConfig<EmailAliasesSettingsUIAndroid> {
 public:
  EmailAliasesSettingsUIAndroidConfig();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_EMAIL_ALIASES_EMAIL_ALIASES_SETTINGS_UI_ANDROID_H_

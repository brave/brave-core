/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_CONTROLLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_CONTROLLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace content {
class WebUI;
}  // namespace content

namespace brave_account {

// Opens the Brave Account dialog on behalf of the rows in brave://settings.
// Unlike the platforms where the rows and the flows share a WebUI, here they do
// not, so the dialog is opened over the settings page rather than by the Brave
// Account WebUI itself - which is why `CloseDialog()` is not implemented here.
class BraveAccountDialogController : public mojom::DialogController {
 public:
  explicit BraveAccountDialogController(content::WebUI* web_ui);

  ~BraveAccountDialogController() override;

 private:
  // brave_account::mojom::DialogController:
  void OpenDialog(const std::string& initiating_service_name) override;
  void CloseDialog() override {}

  const raw_ptr<content::WebUI> web_ui_;
};

}  // namespace brave_account

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_CONTROLLER_H_

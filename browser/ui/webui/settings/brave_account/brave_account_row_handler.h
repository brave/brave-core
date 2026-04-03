/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_ROW_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_ROW_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_account/mojom/brave_account_row.mojom.h"

namespace content {
class WebUI;
}  // namespace content

namespace brave_account {

class BraveAccountRowHandler : public mojom::RowHandler {
 public:
  explicit BraveAccountRowHandler(content::WebUI* web_ui);

  ~BraveAccountRowHandler() override;

 private:
  // brave_account::mojom::RowHandler:
  void OpenDialog(const std::string& initiating_service_name) override;

  const raw_ptr<content::WebUI> web_ui_;
};

}  // namespace brave_account

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_ROW_HANDLER_H_

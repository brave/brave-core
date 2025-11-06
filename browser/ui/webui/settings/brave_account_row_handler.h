/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_ROW_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_ROW_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_account/mojom/brave_account_settings_handler.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace content {
class WebUI;
}  // namespace content

namespace brave_account {

class BraveAccountRowHandler : public mojom::BraveAccountRowHandler {
 public:
  BraveAccountRowHandler(
      mojo::PendingReceiver<mojom::BraveAccountRowHandler> row_handler,
      mojo::PendingRemote<mojom::BraveAccountRow> row,
      content::WebUI* web_ui);

  BraveAccountRowHandler(const BraveAccountRowHandler&) = delete;
  BraveAccountRowHandler& operator=(const BraveAccountRowHandler&) = delete;

  ~BraveAccountRowHandler() override;

 private:
  // brave_account::mojom::BraveAccountRowHandler:
  void GetAccountState(GetAccountStateCallback callback) override;
  void OpenDialog() override;

  mojom::AccountState GetAccountState() const;
  void OnPrefChanged();

  mojo::Receiver<mojom::BraveAccountRowHandler> row_handler_;
  mojo::Remote<mojom::BraveAccountRow> row_;
  const raw_ptr<content::WebUI> web_ui_;
  const raw_ptr<PrefService> pref_service_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace brave_account

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_ROW_HANDLER_H_

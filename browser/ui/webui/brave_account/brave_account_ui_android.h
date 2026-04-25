/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_ANDROID_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_ANDROID_H_

#include "brave/browser/brave_account/brave_account_service_factory.h"
#include "brave/components/brave_account/brave_account_ui_base.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class WebUI;
}  // namespace content

class BraveAccountUIAndroid
    : public BraveAccountUIBase<content::WebUIDataSource,
                                brave_account::BraveAccountServiceFactory>,
      public content::WebUIController,
      public brave_account::mojom::DialogController {
 public:
  using BraveAccountUIBase::BindInterface;

  explicit BraveAccountUIAndroid(content::WebUI* web_ui);

  ~BraveAccountUIAndroid() override;

  void BindInterface(
      mojo::PendingReceiver<brave_account::mojom::DialogController>
          pending_receiver);

 private:
  // brave_account::mojom::DialogController:
  void CloseDialog() override;

  mojo::Receiver<brave_account::mojom::DialogController> receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveAccountUIAndroidConfig
    : public content::DefaultWebUIConfig<BraveAccountUIAndroid> {
 public:
  BraveAccountUIAndroidConfig();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_ANDROID_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/brave_account/brave_account_ui_ios.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "brave/ios/browser/brave_account/brave_account_dialog_opener_bridge.h"
#include "brave/ios/browser/brave_account/dialog_mode_holder.h"
#include "brave/ios/browser/brave_account/dialog_opener_bridge_holder.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "url/gurl.h"

BraveAccountUIIOS::BraveAccountUIIOS(web::WebUIIOS* web_ui, const GURL& url)
    : BraveAccountUIBase(ProfileIOS::FromWebUIIOS(web_ui), url),
      web::WebUIIOSController(web_ui, url.GetHost()) {
  AddInterface<brave_account::mojom::Authentication>();
  AddInterface<brave_account::mojom::DialogController>();
  AddInterface<brave_account::mojom::DialogOpener>();
  AddInterface<password_strength_meter::mojom::PasswordStrengthMeter>();
}

BraveAccountUIIOS::~BraveAccountUIIOS() {
  RemoveInterface<brave_account::mojom::Authentication>();
  RemoveInterface<brave_account::mojom::DialogController>();
  RemoveInterface<brave_account::mojom::DialogOpener>();
  RemoveInterface<password_strength_meter::mojom::PasswordStrengthMeter>();
}

void BraveAccountUIIOS::OpenDialog(
    const std::string& initiating_service_name,
    brave_account::mojom::DialogMode dialog_mode) {
  auto* holder = brave_account::DialogOpenerBridgeHolder::FromWebState(
      web_ui()->GetWebState());
  if (!holder) {
    return;
  }

  [holder->bridge()
      openBraveAccountDialogWithInitiatingServiceName:
          base::SysUTF8ToNSString(initiating_service_name)
                                           dialogMode:
                                               static_cast<
                                                   BraveAccountDialogMode>(
                                                   dialog_mode)];
}

void BraveAccountUIIOS::CloseDialog() {
  web_ui()->GetWebState()->CloseWebState();
}

void BraveAccountUIIOS::GetDialogMode(GetDialogModeCallback callback) {
  auto* holder =
      brave_account::DialogModeHolder::FromWebState(web_ui()->GetWebState());
  std::move(callback).Run(holder ? holder->dialog_mode()
                                 : brave_account::mojom::DialogMode::kDefault);
}

void BraveAccountUIIOS::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DialogController>
        pending_receiver) {
  dialog_controller_receiver_.reset();
  dialog_controller_receiver_.Bind(std::move(pending_receiver));
}

void BraveAccountUIIOS::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DialogOpener>
        pending_receiver) {
  dialog_opener_receiver_.reset();
  dialog_opener_receiver_.Bind(std::move(pending_receiver));
}

template <typename Interface>
void BraveAccountUIIOS::AddInterface() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(static_cast<void (BraveAccountUIIOS::*)(
                              mojo::PendingReceiver<Interface>)>(
                              &BraveAccountUIIOS::BindInterface),
                          base::Unretained(this)));
}

template <typename Interface>
void BraveAccountUIIOS::RemoveInterface() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      Interface::Name_);
}

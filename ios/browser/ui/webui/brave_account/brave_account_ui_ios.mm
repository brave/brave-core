/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/brave_account/brave_account_ui_ios.h"

#include <utility>

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "brave/components/brave_account/brave_account_dependent_services.h"
#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "brave/ios/browser/brave_account/brave_account_dependent_services_factory_ios.h"
#include "brave/ios/browser/brave_account/brave_account_dialog_mode_holder.h"
#include "brave/ios/browser/brave_account/brave_account_screen_opener.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "url/gurl.h"

BraveAccountUIIOS::BraveAccountUIIOS(web::WebUIIOS* web_ui, const GURL& url)
    : BraveAccountUIBase(ProfileIOS::FromWebUIIOS(web_ui)),
      web::WebUIIOSController(web_ui, url.GetHost()) {
  AddInterface<brave_account::mojom::Authentication>();
  AddInterface<brave_account::mojom::DependentServices>();
  AddInterface<brave_account::mojom::DialogController>();
  AddInterface<brave_account::mojom::DialogOpener>();
  AddInterface<brave_account::mojom::SettingsOpener>();
  AddInterface<password_strength_meter::mojom::PasswordStrengthMeter>();
}

BraveAccountUIIOS::~BraveAccountUIIOS() {
  RemoveInterface<brave_account::mojom::Authentication>();
  RemoveInterface<brave_account::mojom::DependentServices>();
  RemoveInterface<brave_account::mojom::DialogController>();
  RemoveInterface<brave_account::mojom::DialogOpener>();
  RemoveInterface<brave_account::mojom::SettingsOpener>();
  RemoveInterface<password_strength_meter::mojom::PasswordStrengthMeter>();
}

void BraveAccountUIIOS::CloseDialog() {
  web_ui()->GetWebState()->CloseWebState();
}

void BraveAccountUIIOS::GetDialogMode(GetDialogModeCallback callback) {
  auto* holder = brave_account::BraveAccountDialogModeHolder::FromWebState(
      web_ui()->GetWebState());
  std::move(callback).Run(holder ? holder->dialog_mode()
                                 : brave_account::mojom::DialogMode::kDefault);
}

void BraveAccountUIIOS::OpenDialog(
    brave_account::mojom::DialogMode dialog_mode) {
  brave_account::OpenBraveAccountDialog(CHECK_DEREF(web_ui()->GetWebState()),
                                        dialog_mode);
}

void BraveAccountUIIOS::OpenSettings(brave_account::mojom::SettingsPage page) {
  brave_account::OpenSettingsPage(CHECK_DEREF(web_ui()->GetWebState()), page);
}

void BraveAccountUIIOS::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DependentServices>
        pending_receiver) {
  CHECK_DEREF(brave_account::BraveAccountDependentServicesFactoryIOS::GetFor(
                  ProfileIOS::FromWebUIIOS(web_ui())))
      .BindInterface(std::move(pending_receiver));
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

void BraveAccountUIIOS::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::SettingsOpener>
        pending_receiver) {
  settings_opener_receiver_.reset();
  settings_opener_receiver_.Bind(std::move(pending_receiver));
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

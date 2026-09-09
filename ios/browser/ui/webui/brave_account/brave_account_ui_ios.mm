/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/brave_account/brave_account_ui_ios.h"

#import <UIKit/UIKit.h>

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_account/brave_account_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "brave/ios/browser/brave_account/brave_account_dialog_presenting.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "net/base/apple/url_conversions.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace {

UIViewController* GetParentControllerFromView(UIView* view) {
  UIResponder* next_responder = [view nextResponder];
  if ([next_responder isKindOfClass:[UIViewController class]]) {
    return static_cast<UIViewController*>(next_responder);
  }

  if ([next_responder isKindOfClass:[UIView class]]) {
    return GetParentControllerFromView(static_cast<UIView*>(next_responder));
  }

  return nil;
}

// The controller owning the page is a generic WebUI host that knows nothing
// about Brave Account, so walk up the presentation chain to whoever presented
// it - the settings screen the rows were opened from. Each step also checks
// the controller's children, since screens are presented wrapped in a
// UINavigationController, which is what `presentingViewController` returns.
id<BraveAccountDialogPresenting> GetDialogPresenterFromView(UIView* view) {
  for (UIViewController* controller = GetParentControllerFromView(view);
       controller; controller = controller.presentingViewController) {
    if ([controller conformsToProtocol:@protocol(BraveAccountDialogPresenting)]) {
      return static_cast<id<BraveAccountDialogPresenting>>(controller);
    }

    for (UIViewController* child in controller.childViewControllers) {
      if ([child conformsToProtocol:@protocol(BraveAccountDialogPresenting)]) {
        return static_cast<id<BraveAccountDialogPresenting>>(child);
      }
    }
  }

  return nil;
}

}  // namespace

BraveAccountUIIOS::BraveAccountUIIOS(web::WebUIIOS* web_ui, const GURL& url)
    : BraveAccountUIBase(ProfileIOS::FromWebUIIOS(web_ui), url),
      web::WebUIIOSController(web_ui, url.GetHost()) {
  AddInterface<brave_account::mojom::Authentication>();
  AddInterface<brave_account::mojom::DialogController>();
  AddInterface<password_strength_meter::mojom::PasswordStrengthMeter>();
}

BraveAccountUIIOS::~BraveAccountUIIOS() {
  RemoveInterface<brave_account::mojom::Authentication>();
  RemoveInterface<brave_account::mojom::DialogController>();
  RemoveInterface<password_strength_meter::mojom::PasswordStrengthMeter>();
}

void BraveAccountUIIOS::OpenDialog(
    const std::string& initiating_service_name) {
  id<BraveAccountDialogPresenting> presenter =
      GetDialogPresenterFromView(web_ui()->GetWebState()->GetView());
  if (!presenter) {
    return;
  }

  const GURL url(kBraveAccountURL);
  const GURL dialog_url =
      initiating_service_name.empty()
          ? url
          : net::AppendQueryParameter(
                url, brave_account::kInitiatingServiceNameQueryParam,
                initiating_service_name);

  [presenter presentBraveAccountDialogForURL:net::NSURLWithGURL(dialog_url)];
}

void BraveAccountUIIOS::CloseDialog() {
  web_ui()->GetWebState()->CloseWebState();
}

void BraveAccountUIIOS::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DialogController>
        pending_receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(pending_receiver));
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

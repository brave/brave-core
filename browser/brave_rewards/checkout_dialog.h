/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_H_

#include <memory>
#include <string>
#include <vector>

#include "content/public/browser/web_ui_message_handler.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace base {
class DictionaryValue;
}

namespace content {
class WebContents;
}

namespace payments {
class PaymentRequest;
}

namespace brave_rewards {

class CheckoutDialogDelegate : public ui::WebDialogDelegate {
 public:
  CheckoutDialogDelegate(base::Value params, payments::PaymentRequest* request);
  ~CheckoutDialogDelegate() override;

  ui::ModalType GetDialogModalType() const override;
  base::string16 GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<content::WebUIMessageHandler*>* handlers) const override;
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnDialogClosed(const std::string& json_retval) override;

  void OnCloseContents(content::WebContents* source,
                       bool* out_close_dialog) override;

  bool ShouldShowDialogTitle() const override;

 private:
  base::Value params_;
  payments::PaymentRequest* request_;

  DISALLOW_COPY_AND_ASSIGN(CheckoutDialogDelegate);
};

class CheckoutDialogHandler : public content::WebUIMessageHandler {
 public:
  explicit CheckoutDialogHandler(payments::PaymentRequest* request);
  ~CheckoutDialogHandler() override;

  // Overridden from WebUIMessageHandler
  void RegisterMessages() override;

 private:
  void HandlePaymentCompletion(const base::ListValue* args);

  payments::PaymentRequest* request_;

  DISALLOW_COPY_AND_ASSIGN(CheckoutDialogHandler);
};

void ShowCheckoutDialog(content::WebContents* initiator,
                        payments::PaymentRequest* request);

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_H_

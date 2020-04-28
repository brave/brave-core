/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/checkout_dialog.h"

#include <memory>
#include <vector>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

using content::WebContents;
using content::WebUIMessageHandler;

namespace {

constexpr int kDialogMinWidth = 548; // 490;
constexpr int kDialogMinHeight = 200;

constexpr int kDialogMaxWidth = 548;
constexpr int kDialogMaxHeight = 800;

class CheckoutDialogDelegate : public ui::WebDialogDelegate {
 public:
  CheckoutDialogDelegate(base::Value params)
      : params_(std::move(params)) {}

  ~CheckoutDialogDelegate() override {}

  ui::ModalType GetDialogModalType() const override {
    // Not used, returning dummy value.
    NOTREACHED();
    return ui::MODAL_TYPE_WINDOW;
  }

  base::string16 GetDialogTitle() const override {
    return base::string16();
  }

  GURL GetDialogContentURL() const override {
    return GURL(kBraveUICheckoutURL);
  }

  void GetWebUIMessageHandlers(
      std::vector<WebUIMessageHandler*>* handlers) const override {
    // BraveCheckoutUI will add message handlers.
  }

  void GetDialogSize(gfx::Size* size) const override {
    // TODO(zenparsing): Is the constrained modal dialog
    // really what we want? It is designed for interfaces
    // that are fixed size for a given screen size.
  }

  std::string GetDialogArgs() const override {
    std::string json;
    base::JSONWriter::Write(params_, &json);
    return json;
  }

  void OnDialogClosed(const std::string& json_retval) override {}

  void OnCloseContents(
      WebContents* source,
      bool* out_close_dialog) override {
    *out_close_dialog = true;
  }

  bool ShouldShowDialogTitle() const override {
    return false;
  }

 private:
  base::Value params_;

  DISALLOW_COPY_AND_ASSIGN(CheckoutDialogDelegate);
};

}  // namespace

namespace brave_rewards {

void ShowCheckoutDialog(WebContents* initiator) {
  // TODO(zenparsing): Take params from caller
  base::Value order_info(base::Value::Type::DICTIONARY);
  order_info.SetStringKey("description", "Some order description");
  order_info.SetDoubleKey("total", 15.0);

  base::Value params(base::Value::Type::DICTIONARY);
  params.SetKey("orderInfo", std::move(order_info));

  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(),
      std::make_unique<CheckoutDialogDelegate>(std::move(params)),
      initiator,
      gfx::Size(kDialogMinWidth, kDialogMinHeight),
      gfx::Size(kDialogMaxWidth, kDialogMaxHeight));
}

}  // namespace brave_rewards

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

constexpr int kDialogWidth = 548;
constexpr int kDialogMinHeight = 200;
constexpr int kDialogMaxHeight = 800;

class CheckoutDialogDelegate : public ui::WebDialogDelegate {
 public:
  CheckoutDialogDelegate(std::unique_ptr<base::DictionaryValue> params)
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
    DCHECK(size);

    // Set the dialog width if it's not set, so that the dialog is
    // center-aligned horizontally when it appears. Avoid setting a
    // dialog height in here as this dialog auto-resizes.
    if (size->IsEmpty())
      size->set_width(kDialogWidth);
  }

  std::string GetDialogArgs() const override {
    std::string json;
    base::JSONWriter::Write(*params_, &json);
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
  std::unique_ptr<base::DictionaryValue> params_;

  DISALLOW_COPY_AND_ASSIGN(CheckoutDialogDelegate);
};

}  // namespace

namespace brave_rewards {

void ShowCheckoutDialog(WebContents* initiator) {
  // TODO(zenparsing): Take typed values from args and create a
  // dictionary value.
  auto params = std::make_unique<base::DictionaryValue>();

  auto delegate = std::make_unique<CheckoutDialogDelegate>(
      std::move(params));

  gfx::Size min_size(kDialogWidth, kDialogMinHeight);
  gfx::Size max_size(kDialogWidth, kDialogMaxHeight);

  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(),
      std::move(delegate),
      initiator,
      min_size,
      max_size);
}

}  // namespace brave_rewards

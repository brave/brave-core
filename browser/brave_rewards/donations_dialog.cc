/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/donations_dialog.h"

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/webui/chrome_web_contents_handler.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

using content::WebContents;
using content::WebUIMessageHandler;

namespace {

constexpr int kDialogMargin = 25;
constexpr int kDialogMinHeight = 400;
constexpr int kDialogMaxHeight = 700;

// A ui::WebDialogDelegate that specifies the donation dialog appearance.
class DonationDialogDelegate : public ui::WebDialogDelegate {
 public:
  explicit DonationDialogDelegate(WebContents* initiator,
                                          std::string publisher_key);
  ~DonationDialogDelegate() override;

  ui::ModalType GetDialogModalType() const override;
  base::string16 GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<WebUIMessageHandler*>* handlers) const override;
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(WebContents* source, bool* out_close_dialog) override;
  bool ShouldShowDialogTitle() const override;

 private:
  WebContents* initiator_;
  const std::string publisher_key_;

  DISALLOW_COPY_AND_ASSIGN(DonationDialogDelegate);
};

DonationDialogDelegate::DonationDialogDelegate(WebContents* initiator,
                                                    std::string publisher_key)
    : initiator_(initiator),
      publisher_key_(publisher_key) {
}

DonationDialogDelegate::~DonationDialogDelegate() {
}

ui::ModalType DonationDialogDelegate::GetDialogModalType() const {
  // Not used, returning dummy value.
  NOTREACHED();
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 DonationDialogDelegate::GetDialogTitle() const {
  // Only used on Windows?
  return base::string16();
}

GURL DonationDialogDelegate::GetDialogContentURL() const {
  return GURL(kBraveUIDonateURL);
}

void DonationDialogDelegate::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* /* handlers */) const {
  // DonationsWebUI should add its own message handlers.
}

void DonationDialogDelegate::GetDialogSize(gfx::Size* size) const {
  DCHECK(size);

  gfx::Size target_size;

  web_modal::WebContentsModalDialogHost* host = nullptr;
  content::WebContents* outermost_web_contents =
      guest_view::GuestViewBase::GetTopLevelWebContents(initiator_);

  Browser* browser = chrome::FindBrowserWithWebContents(outermost_web_contents);
  if (browser)
    host = browser->window()->GetWebContentsModalDialogHost();

  if (host)
    target_size = host->GetMaximumDialogSize();
  else
    target_size = outermost_web_contents->GetContainerBounds().size();
  // initial size in between min and max
  const int max_height =
      kDialogMinHeight + (kDialogMaxHeight - kDialogMinHeight);
  size->SetSize(target_size.width() - kDialogMargin, max_height);
}

std::string DonationDialogDelegate::GetDialogArgs() const {
  std::string data;
  base::DictionaryValue dialog_args;
  dialog_args.SetString("publisherKey", publisher_key_);
  base::JSONWriter::Write(dialog_args, &data);
  return data;
}

void DonationDialogDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {
}

void DonationDialogDelegate::OnCloseContents(WebContents* /* source */,
                                                 bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool DonationDialogDelegate::ShouldShowDialogTitle() const {
  return false;
}

}  // namespace

namespace brave_rewards {

void OpenDonationDialog(WebContents* initiator,
                        const std::string& publisher_key) {
  content::WebContents* outermost_web_contents =
    guest_view::GuestViewBase::GetTopLevelWebContents(initiator);
  gfx::Size host_size = outermost_web_contents->GetContainerBounds().size();
  const int width = host_size.width() - kDialogMargin;
  gfx::Size min_size(width, kDialogMinHeight);
  gfx::Size max_size(width, kDialogMaxHeight);
  // TODO(petemill): adjust min and max when host size changes (e.g. window
  // resize)
  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(),
      std::make_unique<DonationDialogDelegate>(initiator, publisher_key),
      initiator, min_size, max_size);
}

}  // namespace brave_rewards

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/rewards/donations_dialog.h"

#include <string>

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

// A ui::WebDialogDelegate that specifies the donation dialog appearance.
class DonationDialogDelegate : public ui::WebDialogDelegate {
 public:
  explicit DonationDialogDelegate(WebContents* initiator);
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

  DISALLOW_COPY_AND_ASSIGN(DonationDialogDelegate);
};

DonationDialogDelegate::DonationDialogDelegate(WebContents* initiator)
    : initiator_(initiator) {
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

  size->SetSize(target_size.width() - 25, target_size.height() - 25);
}

std::string DonationDialogDelegate::GetDialogArgs() const {
  // todo: specify args in ctor
  // std::string data;
  // base::DictionaryValue dialog_args;
  // dialog_args.SetString("lastEmail", last_email_);
  // dialog_args.SetString("newEmail", new_email_);
  // base::JSONWriter::Write(dialog_args, &data);
  // return data;
  return std::string();
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

} // namespace

namespace donations {

  void OpenDonationDialog(WebContents* initiator) {
    ShowConstrainedWebDialog(initiator->GetBrowserContext(),
                              new DonationDialogDelegate(initiator),
                              initiator);
  }

}
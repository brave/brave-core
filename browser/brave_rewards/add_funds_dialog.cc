// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_rewards/add_funds_dialog.h"

#include <string>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/webui/chrome_web_contents_handler.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/escape.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

using content::WebContents;
using content::WebUIMessageHandler;

namespace {

constexpr int kDialogMargin = 25;
constexpr int kDialogMinHeight = 625;
constexpr int kDialogMaxHeight = 700;
constexpr int kDialogFallbackWidth = 750;

const std::map<std::string, std::string> kCurrencyToNetworkMap {
  {"BTC", "bitcoin"},
  {"BAT", "ethereum"},
  {"ETH", "ethereum"},
  {"LTC", "litecoin"}
};

// A ui::WebDialogDelegate that specifies the AddFunds dialog appearance.
class AddFundsDialogDelegate : public ui::WebDialogDelegate {
 public:
  explicit AddFundsDialogDelegate(
      WebContents* initiator,
      const std::map<std::string, std::string>& addresses);
  ~AddFundsDialogDelegate() override;

  // ui::WebDialogDelegate overrides.
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
  bool HandleContextMenu(const content::ContextMenuParams& params) override;

 private:
  WebContents* initiator_;
  const std::map<std::string, std::string> addresses_;

  DISALLOW_COPY_AND_ASSIGN(AddFundsDialogDelegate);
};

AddFundsDialogDelegate::AddFundsDialogDelegate(
    WebContents* initiator,
    const std::map<std::string, std::string>& addresses)
    : initiator_(initiator), addresses_(addresses) {}

AddFundsDialogDelegate::~AddFundsDialogDelegate() {}

ui::ModalType AddFundsDialogDelegate::GetDialogModalType() const {
  // Not used, returning dummy value.
  NOTREACHED();
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 AddFundsDialogDelegate::GetDialogTitle() const {
  // Irrelevant since we return false in ShouldShowDialogTitle below.
  return base::string16();
}

GURL AddFundsDialogDelegate::GetDialogContentURL() const {
  std::string data = GetDialogArgs();
  base::Base64Encode(data, &data);
  return GURL("https://uphold-widget-uhocggaamg.now.sh/index.html?q=" +
              net::EscapeUrlEncodedData(data, false));
}

void AddFundsDialogDelegate::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* /* handlers */) const {
}

void AddFundsDialogDelegate::GetDialogSize(gfx::Size* size) const {
  DCHECK(size);
  gfx::Size target_size;

  content::WebContents* outermost_web_contents =
      guest_view::GuestViewBase::GetTopLevelWebContents(initiator_);
  if (outermost_web_contents) {
    Browser* browser =
        chrome::FindBrowserWithWebContents(outermost_web_contents);
    if (browser) {
      web_modal::WebContentsModalDialogHost* host =
          browser->window()->GetWebContentsModalDialogHost();
      if (host)
        target_size = host->GetMaximumDialogSize();
    }

    if (target_size.IsEmpty()) {
      target_size = outermost_web_contents->GetContainerBounds().size();
    }
  }

  DCHECK(target_size.width());

  // Initial size in between min and max.
  constexpr int max_height =
      kDialogMinHeight + (kDialogMaxHeight - kDialogMinHeight);

  size->SetSize(target_size.width() ? target_size.width() - kDialogMargin
                                    : kDialogFallbackWidth,
                max_height);
}

std::string AddFundsDialogDelegate::GetDialogArgs() const {
  // Create a list of addresses for serialization.
  auto addresses = std::make_unique<base::DictionaryValue>();
  for (const auto& pair : addresses_) {
    auto address = std::make_unique<base::DictionaryValue>();
    address->SetString("address", pair.second);
    address->SetString("currency", pair.first);
    DCHECK(kCurrencyToNetworkMap.count(pair.first));
    address->SetString("network", kCurrencyToNetworkMap.at(pair.first));
    addresses->SetDictionary(pair.first, std::move(address));
  }

  // Add addresses to the args dictionary.
  base::DictionaryValue dialog_args;
  dialog_args.SetDictionary("addresses", std::move(addresses));

  // Serialize args.
  std::string data;
  base::JSONWriter::Write(dialog_args, &data);
  return data;
}

void AddFundsDialogDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {}

void AddFundsDialogDelegate::OnCloseContents(WebContents* /* source */,
                                             bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool AddFundsDialogDelegate::ShouldShowDialogTitle() const {
  return true;
}

bool AddFundsDialogDelegate::HandleContextMenu(
    const content::ContextMenuParams& params) {
  // Disable context menu.
  return true;
}

}  // namespace

namespace brave_rewards {

void OpenAddFundsDialog(WebContents* initiator,
                        const std::map<std::string, std::string>& addresses) {
  content::WebContents* outermost_web_contents =
      guest_view::GuestViewBase::GetTopLevelWebContents(initiator);
  gfx::Size host_size = outermost_web_contents->GetContainerBounds().size();
  const int width = host_size.width() - kDialogMargin;
  gfx::Size min_size(width, kDialogMinHeight);
  gfx::Size max_size(width, kDialogMaxHeight);
  // TODO: adjust min and max when host size changes (e.g. window resize)
  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(),
      new AddFundsDialogDelegate(initiator, addresses), initiator, min_size,
      max_size);
}

}  // namespace brave_rewards

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
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/extensions/extension_dialog.h"
#include "chrome/browser/ui/views/extensions/extension_dialog_observer.h"
#include "chrome/browser/ui/webui/chrome_web_contents_handler.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/escape.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

// For window

#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/referrer.h"
#include "third_party/blink/public/platform/web_referrer_policy.h"
#include "ui/base/page_transition_types.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"

using content::WebContents;
using content::WebUIMessageHandler;

namespace {

constexpr int kDialogMargin = 25;
constexpr int kDialogMinHeight = 800;
constexpr int kDialogMaxHeight = 700;
constexpr int kDialogFallbackWidth = 900;

const std::map<std::string, std::string> kCurrencyToNetworkMap{
    {"BTC", "bitcoin"},
    {"BAT", "ethereum"},
    {"ETH", "ethereum"},
    {"LTC", "litecoin"}};

std::string GetAddressesAsJSON(
    const std::map<std::string, std::string>& addresses) {
  // Create a dictionary of addresses for serialization.
  auto addresses_dictionary = std::make_unique<base::DictionaryValue>();
  for (const auto& pair : addresses) {
    auto address = std::make_unique<base::DictionaryValue>();
    address->SetString("address", pair.second);
    address->SetString("currency", pair.first);
    DCHECK(kCurrencyToNetworkMap.count(pair.first));
    address->SetString("network", kCurrencyToNetworkMap.at(pair.first));
    addresses_dictionary->SetDictionary(pair.first, std::move(address));
  }

  std::string data;
  base::JSONWriter::Write(*addresses_dictionary, &data);
  return data;
}

std::string ToQueryString(const std::string& data) {
  std::string query_string_value;
  base::Base64Encode(data, &query_string_value);
  return ("addresses=" + net::EscapeUrlEncodedData(query_string_value, false));
}

// A ui::WebDialogDelegate that specifies the AddFunds dialog appearance.
class AddFundsDialogDelegate : public ui::WebDialogDelegate {
 public:
  explicit AddFundsDialogDelegate(
      WebContents* initiator,
      const std::map<std::string, std::string>& addresses,
      brave_rewards::RewardsService* rewards_service);
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
  brave_rewards::RewardsService* rewards_service_;

  DISALLOW_COPY_AND_ASSIGN(AddFundsDialogDelegate);
};

AddFundsDialogDelegate::AddFundsDialogDelegate(
    WebContents* initiator,
    const std::map<std::string, std::string>& addresses,
    brave_rewards::RewardsService* rewards_service)
    : initiator_(initiator),
      addresses_(addresses),
      rewards_service_(rewards_service) {}

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
  return GURL("https://uphold-widget-uhocggaamg.now.sh/index.html?" +
              ToQueryString(GetAddressesAsJSON(addresses_)));
}

void AddFundsDialogDelegate::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* /* handlers */) const {}

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
  return GetAddressesAsJSON(addresses_);
}

void AddFundsDialogDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {
  rewards_service_->GetWalletProperties();
}

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

class AddFundsExtensionDialogObserver : public ExtensionDialogObserver {
 public:
  explicit AddFundsExtensionDialogObserver(
      brave_rewards::RewardsService* rewards_service);
  ~AddFundsExtensionDialogObserver() override;

  // ExtensionDialogObserver interface.
  void ExtensionDialogClosing(ExtensionDialog* popup) override;
  void ExtensionTerminated(ExtensionDialog* popup) override;

 private:
  brave_rewards::RewardsService* rewards_service_;

  DISALLOW_COPY_AND_ASSIGN(AddFundsExtensionDialogObserver);
};

AddFundsExtensionDialogObserver::AddFundsExtensionDialogObserver(
    brave_rewards::RewardsService* rewards_service)
    : rewards_service_(rewards_service) {}

AddFundsExtensionDialogObserver::~AddFundsExtensionDialogObserver() {}

void AddFundsExtensionDialogObserver::ExtensionDialogClosing(
    ExtensionDialog* popup) {
  rewards_service_->GetWalletProperties();
  popup->ObserverDestroyed();
  delete this;
}

void AddFundsExtensionDialogObserver::ExtensionTerminated(
    ExtensionDialog* popup) {
  popup->Close();
}

gfx::Size GetHostSize(WebContents* web_contents) {
  content::WebContents* outermost_web_contents =
      guest_view::GuestViewBase::GetTopLevelWebContents(web_contents);
  return outermost_web_contents->GetContainerBounds().size();
}

gfx::Rect CalculatePopupWindowBounds(WebContents* initiator) {
  // Get bounds of the initiator content to see if they would exceed the minimum
  // size of our popup.
  content::WebContents* outermost_web_contents =
      guest_view::GuestViewBase::GetTopLevelWebContents(initiator);
  gfx::Rect popup_bounds = outermost_web_contents->GetContainerBounds();
  gfx::Size popup_size = popup_bounds.size();
  popup_size.Enlarge(-kDialogMargin, -kDialogMargin);

  // If the initiator is too small, adjust bounds to the minitmum size.
  if (popup_size.width() < kDialogFallbackWidth) {
    popup_bounds.set_x(popup_bounds.x() -
                       (kDialogFallbackWidth - popup_size.width()) / 2);
    popup_bounds.set_width(kDialogFallbackWidth);
  }
  if (popup_size.height() < kDialogMinHeight) {
    popup_bounds.set_y(popup_bounds.y() -
                       (kDialogMinHeight - popup_size.height()) / 2);
    popup_bounds.set_height(kDialogMinHeight);
  }

  // Check if we should move the popup to the center of the screen if it ended
  // up off screen. If the initiator is split between multiple displays this
  // will show the popup on the display that contains the largest chunk of the
  // initiator window.
  display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestView(
          outermost_web_contents->GetNativeView());
  if (!display.bounds().IsEmpty() && !display.bounds().Contains(popup_bounds)) {
    popup_bounds = display.bounds();
    popup_bounds.ClampToCenteredSize(popup_size);
  }

  return popup_bounds;
}

}  // namespace

namespace brave_rewards {

// Open Add Funds as a WebUI dialog.
void OpenAddFundsDialog(WebContents* initiator,
                        const std::map<std::string, std::string>& addresses,
                        brave_rewards::RewardsService* rewards_service) {
  gfx::Size host_size = GetHostSize(initiator);
  const int width = host_size.width() - kDialogMargin;
  gfx::Size min_size(width, kDialogMinHeight);
  gfx::Size max_size(width, kDialogMaxHeight);
  // TODO: adjust min and max when host size changes (e.g. window resize)
  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(),
      new AddFundsDialogDelegate(initiator, addresses, rewards_service),
      initiator, min_size,
      max_size);
}

// Open Add Funds as an extension dialog.
void OpenAddFundsExtensionDialog(
    gfx::NativeWindow parent_window,
    Profile* profile,
    content::WebContents* initiator,
    const std::map<std::string, std::string>& addresses,
        brave_rewards::RewardsService* rewards_service) {
  gfx::Size host_size = GetHostSize(initiator);
  std::string url = std::string("chrome-extension://") +
                    brave_rewards_extension_id +
                    "/brave_rewards_add_funds.html?" +
                    ToQueryString(GetAddressesAsJSON(addresses));
  GURL gurl(url);
  const int width = host_size.width() - kDialogMargin - 100;
  const int height = host_size.height() - kDialogMargin - 100;
  /*ExtensionDialog* dialog = */ ExtensionDialog::Show(
      gurl, parent_window, profile, initiator, width, height, width,
      kDialogMinHeight, L"Bave Rewards",
      new AddFundsExtensionDialogObserver(rewards_service));
}

// Open Add Funds as a popup window.
content::WebContents* OpenAddFundsWindow(content::WebContents* initiator,
    const std::map<std::string, std::string>& addresses) {
  DCHECK(initiator);
  if (!initiator)
    return nullptr;

  content::WebContentsDelegate* wc_delegate = initiator->GetDelegate();
  if (!wc_delegate)
    return nullptr;

  const GURL gurl("https://uphold-widget-uhocggaamg.now.sh/index.html");
  const content::Referrer referrer(GURL("brave://rewards"),
                                   blink::kWebReferrerPolicyAlways);
  content::OpenURLParams params(gurl, referrer,
                                WindowOpenDisposition::NEW_POPUP,
                                ui::PAGE_TRANSITION_LINK, true);

  // Supply addresses via post data. The data is currently in the query string
  // format (application/x-www-form-urlencoded):
  // addresses=UrlEscapedBase64EncodedStringifiedJSON.
  params.uses_post = true;
  const std::string data = ToQueryString(GetAddressesAsJSON(addresses));
  params.post_data = network::ResourceRequestBody::CreateFromBytes(data.data(),
    data.size());

  params.extra_headers = 
    std::string("Content-Type: application/x-www-form-urlencoded\r\n") +
    "Content-Length: " + std::to_string(data.size()) + "\r\n\r\n";

  content::WebContents* newWebContents =
      wc_delegate->OpenURLFromTab(initiator, params);
  if (!newWebContents)
    return nullptr;

  // Reposition/resize the new popup.
  gfx::Rect popup_bounds = CalculatePopupWindowBounds(initiator);
  views::Widget* topLevelWidget =
    views::Widget::GetTopLevelWidgetForNativeView(
      newWebContents->GetNativeView());
  if (topLevelWidget)
    topLevelWidget->SetBounds(popup_bounds);

  return newWebContents;
}

}  // namespace brave_rewards

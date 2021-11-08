/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/webcompat_reporter/webcompat_reporter_dialog.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "url/gurl.h"
#include "url/origin.h"

using content::WebContents;
using content::WebUIMessageHandler;

constexpr int kDialogMinHeight = 100;
constexpr int kDialogMaxHeight = 700;
constexpr int kDialogWidth = 375;

// A ui::WebDialogDelegate that specifies the webcompat reporter's appearance.
class WebcompatReporterDialogDelegate : public ui::WebDialogDelegate {
 public:
  explicit WebcompatReporterDialogDelegate(
      std::unique_ptr<base::DictionaryValue> params);
  WebcompatReporterDialogDelegate(const WebcompatReporterDialogDelegate&) =
      delete;
  WebcompatReporterDialogDelegate& operator=(
      const WebcompatReporterDialogDelegate&) = delete;
  ~WebcompatReporterDialogDelegate() override;

  ui::ModalType GetDialogModalType() const override;
  std::u16string GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<WebUIMessageHandler*>* handlers) const override;
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(WebContents* source, bool* out_close_dialog) override;
  bool ShouldShowDialogTitle() const override;

 private:
  std::unique_ptr<base::DictionaryValue> params_;
};

WebcompatReporterDialogDelegate::WebcompatReporterDialogDelegate(
    std::unique_ptr<base::DictionaryValue> params)
    : params_(std::move(params)) {}

WebcompatReporterDialogDelegate::~WebcompatReporterDialogDelegate() {}

ui::ModalType WebcompatReporterDialogDelegate::GetDialogModalType() const {
  // Not used, returning dummy value.
  NOTREACHED();
  return ui::MODAL_TYPE_WINDOW;
}

std::u16string WebcompatReporterDialogDelegate::GetDialogTitle() const {
  // Only used on Windows?
  return std::u16string();
}

GURL WebcompatReporterDialogDelegate::GetDialogContentURL() const {
  return GURL(kBraveUIWebcompatReporterURL);
}

void WebcompatReporterDialogDelegate::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* /* handlers */) const {
  // WebcompatReporterWebUI should add its own message handlers.
}

void WebcompatReporterDialogDelegate::GetDialogSize(gfx::Size* size) const {
  DCHECK(size);
  size->SetSize(kDialogWidth, kDialogMaxHeight);
}

std::string WebcompatReporterDialogDelegate::GetDialogArgs() const {
  std::string json;
  base::JSONWriter::Write(*params_, &json);
  return json;
}

void WebcompatReporterDialogDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {}

void WebcompatReporterDialogDelegate::OnCloseContents(WebContents* /* source */,
                                                      bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool WebcompatReporterDialogDelegate::ShouldShowDialogTitle() const {
  return false;
}

void OpenWebcompatReporterDialog(content::WebContents* initiator) {
  url::Origin site_url_origin =
      url::Origin::Create(initiator->GetLastCommittedURL());

  auto params_dict = std::make_unique<base::DictionaryValue>();
  params_dict->SetString("siteUrl", site_url_origin.Serialize());

  gfx::Size min_size(kDialogWidth, kDialogMinHeight);
  gfx::Size max_size(kDialogWidth, kDialogMaxHeight);
  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(),
      std::make_unique<WebcompatReporterDialogDelegate>(std::move(params_dict)),
      initiator, min_size, max_size);
}

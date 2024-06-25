/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/webcompat_reporter/webcompat_reporter_dialog.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/ui/brave_shields_data_controller.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom-shared.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/webcompat_reporter/browser/fields.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "url/gurl.h"
#include "url/origin.h"

using content::WebContents;
using content::WebUIMessageHandler;

namespace webcompat_reporter {

namespace {

constexpr int kDialogMinHeight = 100;
constexpr int kDialogMaxHeight = 700;
constexpr int kDialogWidth = 375;

}  // namespace

// A ui::WebDialogDelegate that specifies the webcompat reporter's appearance.
class WebcompatReporterDialogDelegate : public ui::WebDialogDelegate {
 public:
  explicit WebcompatReporterDialogDelegate(base::Value::Dict params);
  WebcompatReporterDialogDelegate(const WebcompatReporterDialogDelegate&) =
      delete;
  WebcompatReporterDialogDelegate& operator=(
      const WebcompatReporterDialogDelegate&) = delete;
  ~WebcompatReporterDialogDelegate() override;

  ui::ModalType GetDialogModalType() const override;
  std::u16string GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<WebUIMessageHandler*>* handlers) override;
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(WebContents* source, bool* out_close_dialog) override;
  bool ShouldShowDialogTitle() const override;
 private:
  base::Value::Dict params_;
};

WebcompatReporterDialogDelegate::WebcompatReporterDialogDelegate(
    base::Value::Dict params)
    : params_(std::move(params)) {}

WebcompatReporterDialogDelegate::~WebcompatReporterDialogDelegate() = default;

ui::ModalType WebcompatReporterDialogDelegate::GetDialogModalType() const {
  // Not used, returning dummy value.
  NOTREACHED_IN_MIGRATION();
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
    std::vector<WebUIMessageHandler*>* /* handlers */) {
  // WebcompatReporterWebUI should add its own message handlers.
}

void WebcompatReporterDialogDelegate::GetDialogSize(gfx::Size* size) const {
  DCHECK(size);
  size->SetSize(kDialogWidth, kDialogMaxHeight);
}

std::string WebcompatReporterDialogDelegate::GetDialogArgs() const {
  std::string json;
  base::JSONWriter::Write(params_, &json);
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

void OpenReporterDialog(content::WebContents* initiator, UISource source) {
  bool shields_enabled = false;
  brave_shields::mojom::FingerprintMode fp_block_mode =
      brave_shields::mojom::FingerprintMode::STANDARD_MODE;
  brave_shields::mojom::AdBlockMode ad_block_mode =
      brave_shields::mojom::AdBlockMode::STANDARD;
  brave_shields::BraveShieldsDataController* shields_data_controller =
      brave_shields::BraveShieldsDataController::FromWebContents(initiator);
  if (shields_data_controller != nullptr) {
    shields_enabled = shields_data_controller->GetBraveShieldsEnabled();
    fp_block_mode = shields_data_controller->GetFingerprintMode();
    ad_block_mode = shields_data_controller->GetAdBlockMode();
  }

  bool is_error_page = false;
  auto* visible_navigation_entry = initiator->GetController().GetVisibleEntry();
  if (visible_navigation_entry) {
    is_error_page = visible_navigation_entry->GetPageType() ==
                    content::PageType::PAGE_TYPE_ERROR;
  }

  // Remove query and fragments from reported URL.
  GURL::Replacements replacements;
  replacements.ClearQuery();
  replacements.ClearRef();
  GURL report_url =
      initiator->GetLastCommittedURL().ReplaceComponents(replacements);

  base::Value::Dict params_dict;
  params_dict.Set(kSiteURLField, report_url.spec());
  params_dict.Set(kShieldsEnabledField, shields_enabled);
  params_dict.Set(kAdBlockSettingField, GetAdBlockModeString(ad_block_mode));
  params_dict.Set(kFPBlockSettingField,
                  GetFingerprintModeString(fp_block_mode));
  params_dict.Set(kUISourceField, static_cast<int>(source));
  params_dict.Set(kIsErrorPage, static_cast<int>(is_error_page));

  gfx::Size min_size(kDialogWidth, kDialogMinHeight);
  gfx::Size max_size(kDialogWidth, kDialogMaxHeight);
  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(),
      std::make_unique<WebcompatReporterDialogDelegate>(std::move(params_dict)),
      initiator, min_size, max_size);
}

}  // namespace webcompat_reporter

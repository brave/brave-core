/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/brave_psst_dialog.h"

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace psst {

namespace {

constexpr int kDialogMinHeight = 100;
constexpr int kDialogMaxHeight = 700;
constexpr int kDialogWidth = 475;

}  // namespace
class PsstDialogDelegate : public ui::WebDialogDelegate {
public:
explicit PsstDialogDelegate(base::Value::Dict params);
PsstDialogDelegate(const PsstDialogDelegate&) = delete;
    PsstDialogDelegate& operator=(
    const PsstDialogDelegate&) = delete;
~PsstDialogDelegate() override;

GURL GetDialogContentURL() const override;
void OnDialogClosed(const std::string& json_retval) override;
void OnCloseContents(content::WebContents* source, bool* out_close_dialog) override;
private:
    base::Value::Dict params_;
};

PsstDialogDelegate::PsstDialogDelegate(
    base::Value::Dict params)
    : params_(std::move(params)) {
set_show_dialog_title(false);
}

PsstDialogDelegate::~PsstDialogDelegate() = default;

GURL PsstDialogDelegate::GetDialogContentURL() const {
    return GURL(kBraveUIPsstURL);
}
void PsstDialogDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {}

void PsstDialogDelegate::OnCloseContents(content::WebContents* /* source */,
                                                      bool* out_close_dialog) {
  *out_close_dialog = true;
}



void OpenPsstDialog(content::WebContents* initiator) {
    base::Value::Dict params_dict;
    // params_dict.Set(kSiteURLField, report_url);
    // params_dict.Set(kShieldsEnabledField, shields_enabled);
    // params_dict.Set(kAdBlockSettingField, adblock_mode);
    // params_dict.Set(kFPBlockSettingField, fingerprint_mode);
    // params_dict.Set(kContactField, contact_info.value_or(""));
    // params_dict.Set(kContactInfoSaveFlagField, contact_info_save_flag);
    // params_dict.Set(kUISourceField, source);
    // params_dict.Set(kIsErrorPage, static_cast<int>(is_error_page));
  
    gfx::Size min_size(kDialogWidth, kDialogMinHeight);
    gfx::Size max_size(kDialogWidth, kDialogMaxHeight);
    ShowConstrainedWebDialogWithAutoResize(
        initiator->GetBrowserContext(),
        std::make_unique<PsstDialogDelegate>(std::move(params_dict)),
        initiator, min_size, max_size);
  
}

}  // namespace psst
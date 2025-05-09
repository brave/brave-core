// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_account/brave_account_dialogs_ui.h"

#include "base/version_info/version_info.h"
#include "brave/components/brave_account/resources/grit/brave_account_resources.h"
#include "brave/components/brave_account/resources/grit/brave_account_resources_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/ui/webui/brave_web_ui_ios_data_source.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/password_manager/core/browser/ui/weak_check_utility.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "url/gurl.h"

namespace {
constexpr char16_t kBraveAccountSelfCustodyLearnMoreURL[] =
    u"https://search.brave.com";
constexpr char16_t kBraveAccountTermsOfServiceURL[] =
    u"https://brave.com/terms-of-use/";
constexpr char16_t kBraveAccountPrivacyAgreementURL[] =
    u"https://brave.com/privacy/browser/";

void AddStringResources(BraveWebUIIOSDataSource* source) {
  static constexpr webui::LocalizedString kStrings[] = {
      // Row:
      {"braveAccountRowTitle", IDS_BRAVE_ACCOUNT_ROW_TITLE},
      {"braveAccountRowDescription", IDS_BRAVE_ACCOUNT_ROW_DESCRIPTION},
      {"braveAccountGetStartedButtonLabel",
       IDS_BRAVE_ACCOUNT_GET_STARTED_BUTTON_LABEL},
      {"braveAccountManageAccountButtonLabel",
       IDS_BRAVE_ACCOUNT_MANAGE_ACCOUNT_BUTTON_LABEL},

      // 'Entry' dialog:
      {"braveAccountEntryDialogTitle", IDS_BRAVE_ACCOUNT_ENTRY_DIALOG_TITLE},
      {"braveAccountEntryDialogDescription",
       IDS_BRAVE_ACCOUNT_ENTRY_DIALOG_DESCRIPTION},
      {"braveAccountCreateBraveAccountButtonLabel",
       IDS_BRAVE_ACCOUNT_ENTRY_DIALOG_CREATE_BRAVE_ACCOUNT_BUTTON_LABEL},
      {"braveAccountAlreadyHaveAccountSignInButtonLabel",
       IDS_BRAVE_ACCOUNT_ALREADY_HAVE_ACCOUNT_SIGN_IN_BUTTON_LABEL},
      {"braveAccountSelfCustodyButtonLabel",
       IDS_BRAVE_ACCOUNT_SELF_CUSTODY_BUTTON_LABEL},

      // 'Create' dialog:
      {"braveAccountCreateDialogTitle", IDS_BRAVE_ACCOUNT_CREATE_DIALOG_TITLE},
      {"braveAccountCreateDialogDescription",
       IDS_BRAVE_ACCOUNT_CREATE_DIALOG_DESCRIPTION},
      {"braveAccountEmailInputErrorMessage",
       IDS_BRAVE_ACCOUNT_EMAIL_INPUT_ERROR_MESSAGE},
      {"braveAccountCreatePasswordInputLabel",
       IDS_BRAVE_ACCOUNT_CREATE_PASSWORD_INPUT_LABEL},
      {"braveAccountPasswordStrengthMeterWeak",
       IDS_BRAVE_ACCOUNT_PASSWORD_STRENGTH_METER_WEAK},
      {"braveAccountPasswordStrengthMeterMedium",
       IDS_BRAVE_ACCOUNT_PASSWORD_STRENGTH_METER_MEDIUM},
      {"braveAccountPasswordStrengthMeterStrong",
       IDS_BRAVE_ACCOUNT_PASSWORD_STRENGTH_METER_STRONG},
      {"braveAccountConfirmPasswordInputLabel",
       IDS_BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_LABEL},
      {"braveAccountConfirmPasswordInputPlaceholder",
       IDS_BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_PLACEHOLDER},
      {"braveAccountConfirmPasswordInputErrorMessage",
       IDS_BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_ERROR_MESSAGE},
      {"braveAccountConfirmPasswordInputSuccessMessage",
       IDS_BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_SUCCESS_MESSAGE},
      {"braveAccountCreateAccountButtonLabel",
       IDS_BRAVE_ACCOUNT_CREATE_ACCOUNT_BUTTON_LABEL},

      // 'Sign In' dialog:
      {"braveAccountSignInDialogTitle", IDS_BRAVE_ACCOUNT_SIGN_IN_DIALOG_TITLE},
      {"braveAccountSignInDialogDescription",
       IDS_BRAVE_ACCOUNT_SIGN_IN_DIALOG_DESCRIPTION},
      {"braveAccountPasswordInputLabel",
       IDS_BRAVE_ACCOUNT_PASSWORD_INPUT_LABEL},
      {"braveAccountForgotPasswordButtonLabel",
       IDS_BRAVE_ACCOUNT_FORGOT_PASSWORD_BUTTON_LABEL},
      {"braveAccountSignInButtonLabel", IDS_BRAVE_ACCOUNT_SIGN_IN_BUTTON_LABEL},

      // 'Forgot Password' dialog:
      {"braveAccountForgotPasswordDialogTitle",
       IDS_BRAVE_ACCOUNT_FORGOT_PASSWORD_DIALOG_TITLE},
      {"braveAccountForgotPasswordDialogDescription",
       IDS_BRAVE_ACCOUNT_FORGOT_PASSWORD_DIALOG_DESCRIPTION},
      {"braveAccountAlertMessage", IDS_BRAVE_ACCOUNT_ALERT_MESSAGE},
      {"braveAccountCancelButtonLabel", IDS_BRAVE_ACCOUNT_CANCEL_BUTTON_LABEL},
      {"braveAccountResetPasswordButtonLabel",
       IDS_BRAVE_ACCOUNT_RESET_PASSWORD_BUTTON_LABEL},

      // Common:
      {"braveAccountEmailInputLabel", IDS_BRAVE_ACCOUNT_EMAIL_INPUT_LABEL},
      {"braveAccountEmailInputPlaceholder",
       IDS_BRAVE_ACCOUNT_EMAIL_INPUT_PLACEHOLDER},
      {"braveAccountPasswordInputPlaceholder",
       IDS_BRAVE_ACCOUNT_PASSWORD_INPUT_PLACEHOLDER},
  };

  source->AddLocalizedStrings(kStrings);

  source->AddString(
      "braveAccountSelfCustodyDescription",
      l10n_util::GetStringFUTF16(IDS_BRAVE_ACCOUNT_SELF_CUSTODY_DESCRIPTION,
                                 kBraveAccountSelfCustodyLearnMoreURL));
  source->AddString(
      "braveAccountConsentCheckboxLabel",
      l10n_util::GetStringFUTF16(IDS_BRAVE_ACCOUNT_CONSENT_CHECKBOX_LABEL,
                                 kBraveAccountTermsOfServiceURL,
                                 kBraveAccountPrivacyAgreementURL));
}
}  // namespace

BraveAccountDialogsUI::BraveAccountDialogsUI(web::WebUIIOS* web_ui,
                                             const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()) {
  BraveWebUIIOSDataSource* source = BraveWebUIIOSDataSource::Create(url.host());

  web::WebUIIOSDataSource::Add(ProfileIOS::FromWebUIIOS(web_ui), source);

  source->AddResourcePaths(kBraveAccountResources);
  source->SetDefaultResource(IDR_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOGS_HTML);

  AddStringResources(source);
  source->UseStringsJs();
  source->EnableReplaceI18nInJS();

  source->AddResourcePath("full_brave_brand.svg",
                          IDR_BRAVE_ACCOUNT_IMAGES_FULL_BRAVE_BRAND_SVG);
  source->AddResourcePath("full_brave_brand_dark.svg",
                          IDR_BRAVE_ACCOUNT_IMAGES_FULL_BRAVE_BRAND_DARK_SVG);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome://resources 'wasm-unsafe-eval' 'self';");

  // so that the XHR that loads the WASM works
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src chrome://resources chrome://theme 'self';");

  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&BraveAccountDialogsUI::BindInterface,
                          base::Unretained(this)));
}

BraveAccountDialogsUI::~BraveAccountDialogsUI() = default;

void BraveAccountDialogsUI::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::BraveAccountHandler>
        pending_receiver) {
  receiver_.Bind(std::move(pending_receiver));
}

void BraveAccountDialogsUI::GetPasswordStrength(
    const std::string& password,
    brave_account::mojom::BraveAccountHandler::GetPasswordStrengthCallback
        callback) {
  std::move(callback).Run(password_manager::GetPasswordStrength(password));
}

void BraveAccountDialogsUI::OpenDialog() {
}

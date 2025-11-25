/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_BASE_H_

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/containers/span.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/resources/grit/brave_account_resources.h"
#include "brave/components/brave_account/resources/grit/brave_account_resources_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/password_strength_meter/password_strength_meter.h"
#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/resource_path.h"
#include "ui/base/webui/web_ui_util.h"

// Template base class for Brave Account WebUI controllers.
//
// `BraveAccountUIBase` encapsulates shared setup logic for Brave Account WebUIs
// across desktop, Android, and iOS. It streamlines the creation and
// configuration of a WebUIDataSource.
//
// Intended to be subclassed with the appropriate WebUIDataSource and
// BraveAccountServiceFactory types.
template <typename WebUIDataSource, typename BraveAccountServiceFactory>
class BraveAccountUIBase {
 public:
  template <typename Profile>
  explicit BraveAccountUIBase(
      Profile* profile,
      base::OnceCallback<void(WebUIDataSource*,
                              base::span<const webui::ResourcePath>,
                              int)> setup_webui_data_source = base::DoNothing())
      : brave_account_service_(
            CHECK_DEREF(BraveAccountServiceFactory::GetFor(profile))) {
    CHECK(brave_account::features::IsBraveAccountEnabled());

    auto* source = WebUIDataSource::CreateAndAdd(profile, kBraveAccountHost);
    std::move(setup_webui_data_source)
        .Run(source, kBraveAccountResources,
             IDR_BRAVE_ACCOUNT_BRAVE_ACCOUNT_PAGE_HTML);
    SetupWebUIDataSource(source);
  }

  void BindInterface(mojo::PendingReceiver<brave_account::mojom::Authentication>
                         pending_receiver) {
    brave_account_service_->BindInterface(std::move(pending_receiver));
  }

  void BindInterface(mojo::PendingReceiver<
                     password_strength_meter::mojom::PasswordStrengthMeter>
                         pending_receiver) {
    password_strength_meter::BindInterface(std::move(pending_receiver));
  }

 private:
  static inline constexpr char16_t kBraveAccountSelfCustodyLearnMoreURL[] =
      u"https://search.brave.com";
  static inline constexpr char16_t kBraveAccountTermsOfServiceURL[] =
      u"https://brave.com/terms-of-use/";
  static inline constexpr char16_t kBraveAccountPrivacyAgreementURL[] =
      u"https://brave.com/privacy/browser/";

  void SetupWebUIDataSource(WebUIDataSource* source) {
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::ScriptSrc,
        "script-src chrome://resources 'self' 'wasm-unsafe-eval';");
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::RequireTrustedTypesFor,
        "require-trusted-types-for 'script';");
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::TrustedTypes,
        "trusted-types lit-html-desktop;");

    source->UseStringsJs();
    source->EnableReplaceI18nInJS();

    source->AddResourcePaths(kBraveAccountResources);
    source->AddResourcePath("", IDR_BRAVE_ACCOUNT_BRAVE_ACCOUNT_PAGE_HTML);

    static constexpr webui::LocalizedString kStrings[] = {
        {"braveAccountPageTitle", IDS_BRAVE_ACCOUNT_PAGE_TITLE},
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
        {"braveAccountCreateDialogTitle",
         IDS_BRAVE_ACCOUNT_CREATE_DIALOG_TITLE},
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
        {"braveAccountSignInDialogTitle",
         IDS_BRAVE_ACCOUNT_SIGN_IN_DIALOG_TITLE},
        {"braveAccountSignInDialogDescription",
         IDS_BRAVE_ACCOUNT_SIGN_IN_DIALOG_DESCRIPTION},
        {"braveAccountPasswordInputLabel",
         IDS_BRAVE_ACCOUNT_PASSWORD_INPUT_LABEL},
        {"braveAccountForgotPasswordButtonLabel",
         IDS_BRAVE_ACCOUNT_FORGOT_PASSWORD_BUTTON_LABEL},
        {"braveAccountSignInButtonLabel",
         IDS_BRAVE_ACCOUNT_SIGN_IN_BUTTON_LABEL},
        // 'Forgot Password' dialog:
        {"braveAccountForgotPasswordDialogTitle",
         IDS_BRAVE_ACCOUNT_FORGOT_PASSWORD_DIALOG_TITLE},
        {"braveAccountForgotPasswordDialogDescription",
         IDS_BRAVE_ACCOUNT_FORGOT_PASSWORD_DIALOG_DESCRIPTION},
        {"braveAccountAlertMessage", IDS_BRAVE_ACCOUNT_ALERT_MESSAGE},
        {"braveAccountResetPasswordButtonLabel",
         IDS_BRAVE_ACCOUNT_RESET_PASSWORD_BUTTON_LABEL},
        // 'Error' dialog:
        {"braveAccountErrorDialogTitle", IDS_BRAVE_ACCOUNT_ERROR_DIALOG_TITLE},
        {"braveAccountErrorDialogDescription",
         IDS_BRAVE_ACCOUNT_ERROR_DIALOG_DESCRIPTION},
        {"braveAccountErrorDialogError", IDS_BRAVE_ACCOUNT_ERROR_DIALOG_ERROR},
        {"braveAccountErrorDialogClientError",
         IDS_BRAVE_ACCOUNT_ERROR_DIALOG_CLIENT_ERROR},
        {"braveAccountErrorDialogServerError",
         IDS_BRAVE_ACCOUNT_ERROR_DIALOG_SERVER_ERROR},
        {"braveAccountErrorDialogIncorrectEmail",
         IDS_BRAVE_ACCOUNT_ERROR_DIALOG_INCORRECT_EMAIL},
        {"braveAccountErrorDialogIncorrectPassword",
         IDS_BRAVE_ACCOUNT_ERROR_DIALOG_INCORRECT_PASSWORD},
        {"braveAccountErrorDialogAccountExists",
         IDS_BRAVE_ACCOUNT_ERROR_DIALOG_ACCOUNT_EXISTS},
        {"braveAccountErrorDialogEmailDomainNotSupported",
         IDS_BRAVE_ACCOUNT_ERROR_DIALOG_EMAIL_DOMAIN_NOT_SUPPORTED},
        {"braveAccountErrorDialogTooManyVerifications",
         IDS_BRAVE_ACCOUNT_ERROR_DIALOG_TOO_MANY_VERIFICATIONS},
        // Common:
        {"braveAccountBackButtonLabel", IDS_BRAVE_ACCOUNT_BACK_BUTTON_LABEL},
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

    source->AddResourcePath("full_brave_brand.svg",
                            IDR_BRAVE_ACCOUNT_IMAGES_FULL_BRAVE_BRAND_SVG);
    source->AddResourcePath("full_brave_brand_dark.svg",
                            IDR_BRAVE_ACCOUNT_IMAGES_FULL_BRAVE_BRAND_DARK_SVG);
  }

 private:
  const raw_ref<brave_account::BraveAccountService> brave_account_service_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_BASE_H_

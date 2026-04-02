// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/autofill/chrome_autofill_client.h"

#include "base/check_is_test.h"
#include "base/memory/ptr_util.h"
#include "brave/browser/ui/email_aliases/email_aliases_controller.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/autofill/payments/webauthn_dialog_controller_impl.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "components/autofill/content/browser/renderer_forms_from_browser_form.h"
#include "components/autofill/core/browser/form_import/form_data_importer.h"
#include "components/autofill/core/browser/foundations/browser_autofill_manager.h"
#include "components/grit/brave_components_strings.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/strike_database/strike_database.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "ui/base/l10n/l10n_util.h"

namespace autofill {

namespace {

bool IsPrivateProfile(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }
  auto* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (!profile) {
    return false;
  }
  return (profile_metrics::GetBrowserProfileType(profile) ==
          profile_metrics::BrowserProfileType::kIncognito) ||
         profile->IsTor();
}

email_aliases::EmailAliasesController* GetEmailAliasesControllerFromWebContents(
    content::WebContents* web_contents) {
  tabs::TabInterface* tab =
      tabs::TabInterface::MaybeGetFromContents(web_contents);
  if (!tab) {
    return nullptr;
  }
  BrowserWindowInterface* window_interface = tab->GetBrowserWindowInterface();
  if (!window_interface) {
    return nullptr;
  }
  return window_interface->GetFeatures().email_aliases_controller();
}

}  // namespace

class BraveChromeAutofillClient : public ChromeAutofillClient {
 public:
  using ChromeAutofillClient::ChromeAutofillClient;

  static std::unique_ptr<ChromeAutofillClient> CreateForTesting(
      content::WebContents* contents) {
    CHECK_IS_TEST();
    return base::WrapUnique(new BraveChromeAutofillClient(contents));
  }

  AutofillOptimizationGuideDecider* GetAutofillOptimizationGuideDecider()
      const override {
    if (optimization_guide::features::IsOptimizationHintsEnabled()) {
      return ChromeAutofillClient::GetAutofillOptimizationGuideDecider();
    }
    return nullptr;
  }

  bool IsAutocompleteEnabled() const override {
    auto enabled = ChromeAutofillClient::IsAutocompleteEnabled();
    if (!IsPrivateProfile(web_contents())) {
      return enabled;
    }
    enabled = enabled && GetPrefs()->GetBoolean(kBraveAutofillPrivateWindows);
    return enabled;
  }

  bool IsAutofillEnabled() const override {
    auto enabled = ChromeAutofillClient::IsAutofillEnabled();
    if (GetProfileType() != profile_metrics::BrowserProfileType::kIncognito &&
        GetProfileType() !=
            profile_metrics::BrowserProfileType::kOtherOffTheRecordProfile) {
      return enabled;
    }
    enabled = enabled && GetPrefs()->GetBoolean(kBraveAutofillPrivateWindows);
    return enabled;
  }

  void BraveAddSuggestions(
      const PasswordFormClassification& form_classification,
      const FormFieldData& field,
      std::vector<Suggestion>& chrome_suggestions) override {
#if !BUILDFLAG(IS_ANDROID)
    AddEmailAliasSuggestsion(form_classification, field, chrome_suggestions);
#endif
  }

  bool BraveHandleSuggestion(const Suggestion& suggestion,
                             const autofill::FieldGlobalId& field) override {
#if !BUILDFLAG(IS_ANDROID)
    if (HandleEmailAliasSuggestsion(suggestion, field)) {
      return true;
    }
#endif
    return false;
  }

 private:
#if !BUILDFLAG(IS_ANDROID)
  void AddEmailAliasSuggestsion(
      const PasswordFormClassification& form_classification,
      const FormFieldData& field,
      std::vector<Suggestion>& chrome_suggestions) {
    email_aliases::EmailAliasesController* controller =
        GetEmailAliasesControllerFromWebContents(web_contents());
    if (controller) {
      const bool contains_email_suggestion =
          std::ranges::find_if(chrome_suggestions, [](const auto& suggestion) {
            return suggestion.icon == autofill::Suggestion::Icon::kEmail;
          }) != chrome_suggestions.end();

      const bool username_field_in_sign_up_form =
          form_classification.type ==
              PasswordFormClassification::Type::kSignupForm &&
          (form_classification.username_field == field.global_id() ||
           field.form_control_type() == FormControlType::kInputEmail);

      if (contains_email_suggestion || username_field_in_sign_up_form) {
        autofill::Suggestion new_email_alias(
            autofill::SuggestionType::kManageAddress);
        new_email_alias.icon = autofill::Suggestion::Icon::kEmail;
        new_email_alias.main_text = autofill::Suggestion::Text(
            l10n_util::GetStringUTF16(IDS_IDC_NEW_EMAIL_ALIAS));
        new_email_alias.brave_new_email_alias_suggestion = true;
        chrome_suggestions.push_back(std::move(new_email_alias));
      }
    }
  }

  bool HandleEmailAliasSuggestsion(const Suggestion& suggestion,
                                   const autofill::FieldGlobalId& field) {
    if (!suggestion.brave_new_email_alias_suggestion) {
      return false;
    }
    email_aliases::EmailAliasesController* email_aliases =
        GetEmailAliasesControllerFromWebContents(web_contents());
    if (email_aliases) {
      auto* render_frame_host = autofill::FindRenderFrameHostByToken(
          *web_contents(), field.frame_token);
      if (render_frame_host) {
        email_aliases->ShowBubble(web_contents(), render_frame_host,
                                  field.renderer_id.value());
      }
    }
    return true;
  }
#endif
};

std::unique_ptr<ChromeAutofillClient> CreateBraveChromeAutofillClientForTesting(
    content::WebContents* contents) {
  CHECK_IS_TEST();
  return BraveChromeAutofillClient::CreateForTesting(contents);
}

}  // namespace autofill

#define WrapUnique WrapUnique(new autofill::BraveChromeAutofillClient(web_contents))); \
  if (0) std::unique_ptr<autofill::ChromeAutofillClient> dummy(
#include <chrome/browser/ui/autofill/chrome_autofill_client.cc>
#undef WrapUnique

namespace autofill {

AutofillOptimizationGuideDecider*
ChromeAutofillClient::GetAutofillOptimizationGuideDecider_Unused() const {
  return nullptr;
}

bool ChromeAutofillClient::IsAutofillEnabled_Unused() const {
  return false;
}

bool ChromeAutofillClient::IsAutocompleteEnabled_Unused() const {
  return false;
}

}  // namespace autofill

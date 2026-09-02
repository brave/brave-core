// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/autofill/chrome_autofill_client.h"

#include <optional>

#include "base/check_is_test.h"
#include "base/memory/ptr_util.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/email_aliases/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/autofill/autofill_suggestion_controller_utils.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "components/autofill/content/browser/renderer_forms_from_browser_form.h"
#include "components/autofill/core/browser/form_import/form_data_importer.h"
#include "components/autofill/core/browser/foundations/browser_autofill_manager.h"
#include "components/autofill/core/browser/webdata/autocomplete/autocomplete_entry.h"
#include "components/grit/brave_components_strings.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/strike_database/strike_database.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
#include "brave/browser/email_aliases/email_aliases_service_factory.h"
#include "brave/browser/ui/email_aliases/email_aliases_controller.h"
#include "brave/components/email_aliases/email_aliases_service.h"
#include "brave/components/email_aliases/pref_names.h"
#endif

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

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
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
#endif  // BUILDFLAG(ENABLE_EMAIL_ALIASES)

}  // namespace

class BraveChromeAutofillClient : public ChromeAutofillClient {
 public:
  using ChromeAutofillClient::ChromeAutofillClient;

  static std::unique_ptr<ChromeAutofillClient> CreateForTesting(  // IN-TEST
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
#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
    AddEmailAliasSuggestsion(form_classification, field, chrome_suggestions);
#endif
  }

  bool BraveHandleSuggestion(const Suggestion& suggestion,
                             const autofill::FieldGlobalId& field) override {
#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
    if (HandleEmailAliasSuggestsion(suggestion, field)) {
      return true;
    }
#endif
    return false;
  }

 private:
#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
  std::optional<Suggestion> GetYourEmailAliasesSuggestions() {
    auto* profile =
        Profile::FromBrowserContext(web_contents()->GetBrowserContext());
    if (!profile) {
      return std::nullopt;
    }
    auto* service =
        email_aliases::EmailAliasesServiceFactory::GetServiceForProfile(
            profile);
    if (!service) {
      return std::nullopt;
    }

    const auto& aliases = service->aliases();
    if (aliases.empty()) {
      return std::nullopt;
    }

    Suggestion your_email_aliases(
        autofill::SuggestionType::kDevtoolsTestAddresses);
    your_email_aliases.main_text = autofill::Suggestion::Text(
        l10n_util::GetStringUTF16(IDS_IDC_YOUR_EMAIL_ALIASES));
    your_email_aliases.acceptability =
        Suggestion::Acceptability::kSelectableButUnacceptable;

    for (const email_aliases::mojom::AliasPtr& alias : aliases) {
      autofill::Suggestion entry(autofill::SuggestionType::kAutocompleteEntry);
      entry.main_text =
          autofill::Suggestion::Text(base::UTF8ToUTF16(alias->email));
      entry.labels.push_back({autofill::Suggestion::Text(
          base::UTF8ToUTF16(alias->note.value_or(std::string{})))});
      entry.payload = AutocompleteEntry{};
      entry.brave_email_alias_suggestion = true;
      your_email_aliases.children.push_back(std::move(entry));
    }

    return your_email_aliases;
  }

  void AddEmailAliasSuggestsion(
      const PasswordFormClassification& form_classification,
      const FormFieldData& field,
      std::vector<Suggestion>& chrome_suggestions) {
    email_aliases::EmailAliasesController* controller =
        GetEmailAliasesControllerFromWebContents(web_contents());
    if (controller) {
      auto* profile =
          Profile::FromBrowserContext(web_contents()->GetBrowserContext());
      if (!profile->GetPrefs()->GetBoolean(
              email_aliases::prefs::
                  kEmailAliasesNewAliasAutofillSuggestionEnabled)) {
        return;
      }

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
            autofill::SuggestionType::kAddressEntry);
        new_email_alias.icon = autofill::Suggestion::Icon::kEmail;
        new_email_alias.main_text = autofill::Suggestion::Text(
            l10n_util::GetStringUTF16(IDS_IDC_NEW_EMAIL_ALIAS));
        new_email_alias.labels.push_back({autofill::Suggestion::Text(
            l10n_util::GetStringUTF16(IDS_IDC_NEW_EMAIL_ALIAS_DESC))});
        new_email_alias.brave_new_email_alias_suggestion = true;

        size_t insert_index = chrome_suggestions.size();
        for (size_t i = 0; i < chrome_suggestions.size(); ++i) {
          if (IsFooterItem(chrome_suggestions, i)) {
            insert_index = i;
            break;
          }
        }
        chrome_suggestions.insert(chrome_suggestions.begin() + insert_index,
                                  std::move(new_email_alias));
        if (auto your_email_aliases = GetYourEmailAliasesSuggestions()) {
          chrome_suggestions.insert(chrome_suggestions.begin() + insert_index,
                                    std::move(*your_email_aliases));
        }
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
        email_aliases->ShowBubble(
            web_contents(), render_frame_host, field.renderer_id.value(),
            email_aliases::SettingsPageMethod::kAutofillBubble);
      }
    }
    return true;
  }
#endif  // BUILDFLAG(ENABLE_EMAIL_ALIASES)
};

std::unique_ptr<ChromeAutofillClient>
CreateBraveChromeAutofillClientForTesting(  // IN-TEST
    content::WebContents* contents) {
  CHECK_IS_TEST();
  return BraveChromeAutofillClient::CreateForTesting(contents);  // IN-TEST
}

}  // namespace autofill

#include <chrome/browser/ui/autofill/chrome_autofill_client.cc>

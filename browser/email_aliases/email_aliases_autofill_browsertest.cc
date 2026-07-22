/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <string_view>
#include <vector>

#include "base/path_service.h"
#include "base/test/run_until.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/email_aliases/features.h"
#include "brave/components/email_aliases/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/autofill/autofill_uitest_util.h"
#include "chrome/browser/policy/policy_test_utils.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/content/browser/test_autofill_client_injector.h"
#include "components/autofill/content/browser/test_autofill_manager_injector.h"
#include "components/autofill/core/browser/foundations/autofill_manager.h"
#include "components/autofill/core/browser/foundations/browser_autofill_manager.h"
#include "components/autofill/core/browser/foundations/test_autofill_manager_waiter.h"
#include "components/autofill/core/browser/suggestions/suggestion.h"
#include "components/autofill/core/browser/test_utils/autofill_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/base/l10n/l10n_util.h"

namespace email_aliases {

namespace {

void ExpectBraveEmailAliasAddressEntry(const autofill::Suggestion& suggestion) {
  EXPECT_EQ(suggestion.type, autofill::SuggestionType::kAddressEntry);
  EXPECT_TRUE(suggestion.brave_new_email_alias_suggestion);
  EXPECT_EQ(suggestion.main_text.value,
            l10n_util::GetStringUTF16(IDS_IDC_NEW_EMAIL_ALIAS));
  ASSERT_EQ(1u, suggestion.labels.size());
  EXPECT_EQ(suggestion.labels[0][0].value,
            l10n_util::GetStringUTF16(IDS_IDC_NEW_EMAIL_ALIAS_DESC));
  EXPECT_EQ(suggestion.icon, autofill::Suggestion::Icon::kEmail);
}

const autofill::Suggestion* FindBraveEmailAliasSuggestion(
    const std::vector<autofill::Suggestion>& suggestions) {
  const auto it = std::ranges::find_if(
      suggestions, &autofill::Suggestion::brave_new_email_alias_suggestion);
  return it == suggestions.end() ? nullptr : &*it;
}

}  // namespace

class TestAutofillClient : public autofill::ChromeAutofillClient {
 public:
  explicit TestAutofillClient(content::WebContents* web_contents)
      : autofill::ChromeAutofillClient(web_contents),
        brave_autofill_client_(
            autofill::CreateBraveChromeAutofillClientForTesting(web_contents)) {
  }

  SuggestionUiSessionId ShowAutofillSuggestions(
      const autofill::AutofillClient::PopupOpenArgs& open_args,
      base::WeakPtr<autofill::AutofillSuggestionDelegate> delegate) override {
    suggestions_ = open_args.suggestions;
    return autofill::ChromeAutofillClient::ShowAutofillSuggestions(open_args,
                                                                   delegate);
  }

  const std::vector<autofill::Suggestion>& suggestions() const {
    return suggestions_;
  }

  void ResetSuggestions() { suggestions_ = {}; }

  void BraveAddSuggestions(
      const autofill::PasswordFormClassification& form_classification,
      const autofill::FormFieldData& field,
      std::vector<autofill::Suggestion>& chrome_suggestions) override {
    brave_autofill_client_->BraveAddSuggestions(form_classification, field,
                                                chrome_suggestions);
  }

  bool BraveHandleSuggestion(const autofill::Suggestion& suggestion,
                             const autofill::FieldGlobalId& field) override {
    return brave_autofill_client_->BraveHandleSuggestion(suggestion, field);
  }

 private:
  std::unique_ptr<autofill::ChromeAutofillClient> brave_autofill_client_;
  std::vector<autofill::Suggestion> suggestions_;
};

class TestAutofillManager : public autofill::BrowserAutofillManager {
 public:
  explicit TestAutofillManager(autofill::ContentAutofillDriver* driver)
      : autofill::BrowserAutofillManager(driver) {}

  void WaitForFormsSeen() { ASSERT_TRUE(forms_seen_waiter_.Wait()); }

  void WaitForAskForValuesToFill() {
    ASSERT_TRUE(ask_for_value_to_fill_waiter_.Wait());
  }

 private:
  autofill::TestAutofillManagerWaiter forms_seen_waiter_{
      *this,
      {autofill::AutofillManagerEvent::kFormsSeen}};
  autofill::TestAutofillManagerWaiter ask_for_value_to_fill_waiter_{
      *this,
      {autofill::AutofillManagerEvent::kAskForValuesToFill}};
};

class EmailAliasesAutofillTest : public InProcessBrowserTest,
                                 public testing::WithParamInterface<bool> {
 public:
  EmailAliasesAutofillTest() {
    if (GetParam()) {
      feature_list_.InitAndEnableFeature(features::kEmailAliases);
    } else {
      feature_list_.InitAndDisableFeature(features::kEmailAliases);
    }
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    autofill::WaitForPersonalDataManagerToBeLoaded(browser()->profile());

    embedded_test_server()->ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ImportAddress() {
    autofill::AddTestProfile(browser()->profile(),
                             autofill::test::GetFullProfile());
  }

  void NavigateToTestPage(std::string_view path) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL(path)));
    content::MainThreadFrameObserver frame_observer(
        GetWebContents()->GetRenderWidgetHostView()->GetRenderWidgetHost());
    frame_observer.Wait();
  }

 protected:
  TestAutofillClient* autofill_client() {
    return autofill_client_injector_[GetWebContents()];
  }

  TestAutofillManager* autofill_manager() {
    return autofill_manager_injector_[GetWebContents()];
  }

 private:
  autofill::TestAutofillClientInjector<TestAutofillClient>
      autofill_client_injector_;

  autofill::TestAutofillManagerInjector<TestAutofillManager>
      autofill_manager_injector_;

  base::test::ScopedFeatureList feature_list_;
};

INSTANTIATE_TEST_SUITE_P(, EmailAliasesAutofillTest, testing::Bool());

IN_PROC_BROWSER_TEST_P(EmailAliasesAutofillTest, NewEmailAliasSuggestion) {
  ImportAddress();
  NavigateToTestPage("/email_aliases/inputs.html");
  autofill_manager()->WaitForFormsSeen();

  content::SimulateMouseClickOrTapElementWithId(GetWebContents(), "type-email");

  autofill_manager()->WaitForAskForValuesToFill();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !autofill_client()->suggestions().empty(); }));

  ASSERT_LE(3u, autofill_client()->suggestions().size());

  // Saved profile suggestion — kAddressEntry without the Brave flag.
  EXPECT_EQ(autofill_client()->suggestions()[0].main_text.value,
            u"johndoe@hades.com");
  EXPECT_EQ(autofill_client()->suggestions()[0].type,
            autofill::SuggestionType::kAddressEntry);
  EXPECT_FALSE(
      autofill_client()->suggestions()[0].brave_new_email_alias_suggestion);

  const autofill::Suggestion* brave_suggestion =
      FindBraveEmailAliasSuggestion(autofill_client()->suggestions());
  if (GetParam()) {
    ASSERT_LE(4u, autofill_client()->suggestions().size());
    ASSERT_NE(brave_suggestion, nullptr);

    // Brave email alias suggestion — last body row, before footer.
    ExpectBraveEmailAliasAddressEntry(*brave_suggestion);
    EXPECT_EQ(brave_suggestion, &autofill_client()->suggestions()[1]);

    EXPECT_EQ(autofill_client()->suggestions()[2].type,
              autofill::SuggestionType::kSeparator);
    EXPECT_EQ(autofill_client()->suggestions()[3].type,
              autofill::SuggestionType::kManageAddress);
  } else {
    // Feature is disabled — no Brave email alias suggestion.
    EXPECT_EQ(brave_suggestion, nullptr);

    EXPECT_EQ(3u, autofill_client()->suggestions().size());
    EXPECT_EQ(autofill_client()->suggestions()[1].type,
              autofill::SuggestionType::kSeparator);
    EXPECT_EQ(autofill_client()->suggestions()[2].type,
              autofill::SuggestionType::kManageAddress);
  }

  autofill_client()->ResetSuggestions();
}

IN_PROC_BROWSER_TEST_P(EmailAliasesAutofillTest,
                       NewEmailAliasSuggestionSignup) {
  NavigateToTestPage("/email_aliases/signup_form.html");
  autofill_manager()->WaitForFormsSeen();

  content::SimulateMouseClickOrTapElementWithId(GetWebContents(), "email");

  autofill_manager()->WaitForAskForValuesToFill();
  if (!GetParam()) {
    EXPECT_TRUE(autofill_client()->suggestions().empty());
    return;
  }

  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !autofill_client()->suggestions().empty(); }));

  // Brave email alias suggestion as the only entry on signup email field.
  EXPECT_EQ(1u, autofill_client()->suggestions().size());

  const autofill::Suggestion* brave_suggestion =
      FindBraveEmailAliasSuggestion(autofill_client()->suggestions());
  ASSERT_NE(brave_suggestion, nullptr);
  ExpectBraveEmailAliasAddressEntry(*brave_suggestion);

  autofill_client()->ResetSuggestions();

  // Disable "New Email Alias" suggestion.
  browser()->profile()->GetPrefs()->SetBoolean(
      prefs::kEmailAliasesNewAliasAutofillSuggestionEnabled, false);

  content::SimulateMouseClickOrTapElementWithId(GetWebContents(), "email");
  autofill_manager()->WaitForAskForValuesToFill();
  EXPECT_TRUE(autofill_client()->suggestions().empty());

  autofill_client()->ResetSuggestions();
}

}  // namespace email_aliases

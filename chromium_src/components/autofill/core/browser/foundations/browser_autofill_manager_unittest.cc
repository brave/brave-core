/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/foundations/browser_autofill_manager_unittest.cc"

namespace autofill {
namespace {

// Test that if a form is mixed content we show a warning instead of any
// suggestions.
TEST_F(BrowserAutofillManagerTest, Onion_MixedForm1) {
  // Set up our form data.
  FormData form;
  form.set_name(u"MyForm");
  form.set_url(GURL("https://myform.onion/form.html"));
  form.set_action(GURL("http://myform.com/submit.html"));
  form.set_fields({CreateTestFormField("Name on Card", "nameoncard", "",
                                       FormControlType::kInputText)});

  OnAskForValuesToFill(form, form.fields()[0]);

  // Test that we sent the right values to the external delegate.
  external_delegate()->CheckSuggestions(
      form.fields().back().global_id(),
      {Suggestion(l10n_util::GetStringUTF8(IDS_AUTOFILL_WARNING_MIXED_FORM), "",
                  Suggestion::Icon::kNoIcon,
                  SuggestionType::kMixedFormMessage)});
}

// Test that if a form is mixed content we show a warning instead of any
// suggestions. A .onion hostname is considered secure.
TEST_F(BrowserAutofillManagerTest, Onion_MixedForm2) {
  // Set up our form data.
  FormData form;
  form.set_name(u"MyForm");
  form.set_url(GURL("http://myform.onion/form.html"));
  form.set_action(GURL("http://myform.com/submit.html"));
  form.set_fields({CreateTestFormField("Name on Card", "nameoncard", "",
                                       FormControlType::kInputText)});

  OnAskForValuesToFill(form, form.fields()[0]);

  // Test that we sent the right values to the external delegate.
  external_delegate()->CheckSuggestions(
      form.fields().back().global_id(),
      {Suggestion(l10n_util::GetStringUTF8(IDS_AUTOFILL_WARNING_MIXED_FORM), "",
                  Suggestion::Icon::kNoIcon,
                  SuggestionType::kMixedFormMessage)});
}

// Test that if a form is not mixed content we show suggestions.
TEST_F(BrowserAutofillManagerTest, Onion_NonMixedForm) {
  // Set up our form data.
  FormData form;
  form.set_name(u"MyForm");
  form.set_url(GURL("http://myform.onion/form.html"));
  form.set_action(GURL("https://myform.com/submit.html"));
  form.set_fields({CreateTestFormField("Name on Card", "nameoncard", "",
                                       FormControlType::kInputText)});

  OnAskForValuesToFill(form, form.fields()[0]);

  // Check there is no warning.
  EXPECT_FALSE(external_delegate()->on_suggestions_returned_seen());
}

}  // namespace
}  // namespace autofill

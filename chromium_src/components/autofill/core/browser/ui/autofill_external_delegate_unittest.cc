/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/autofill/core/browser/ui/autofill_external_delegate_unittest.cc>

namespace autofill {
namespace {

class BraveMockAutofillClient : public MockAutofillClient {
 public:
  MOCK_METHOD(bool,
              BraveHandleSuggestion,
              (const Suggestion&, const FieldGlobalId&),
              (override));
};

class BraveAutofillExternalDelegateTest
    : public testing::Test,
      public WithTestAutofillClientDriverManager<
          NiceMock<BraveMockAutofillClient>,
          NiceMock<MockAutofillDriver>,
          NiceMock<MockBrowserAutofillManager>,
          MockPaymentsAutofillClient> {
 protected:
  void SetUp() override {
    InitAutofillClient();
    CreateAutofillDriver();
  }

  void TearDown() override { DestroyAutofillClient(); }

  TestExternalDelegate& external_delegate() {
    return static_cast<TestExternalDelegate&>(
        *test_api(autofill_manager()).external_delegate());
  }

 private:
  base::test::TaskEnvironment task_environment_;
  test::AutofillUnitTestEnvironment autofill_test_environment_;
};

// The email alias bubble is anchored on the field Brave gets here, which must
// be the field the suggestion was accepted on, not the last queried one.
TEST_F(BraveAutofillExternalDelegateTest, HandleSuggestionGetsAcceptedField) {
  FormData form = test::GetFormData(
      {.fields = {{.role = EMAIL_ADDRESS}, {.role = EMAIL_ADDRESS}}});
  autofill_manager().OnFormsSeen({form}, {},
                                 AutofillManagerTestApi::pass_key());
  external_delegate().OnQuery(form, form.fields()[0], gfx::Rect(),
                              kDefaultSuggestionTriggerSource);

  const FieldGlobalId accepted_field_id = form.fields()[1].global_id();
  EXPECT_CALL(autofill_client(), BraveHandleSuggestion(_, accepted_field_id))
      .WillOnce(Return(true));
  external_delegate().DidAcceptSuggestion(
      Suggestion(u"alias@example.com", SuggestionType::kAddressEntry),
      /*metadata=*/{}, form.global_id(), accepted_field_id);
}

}  // namespace
}  // namespace autofill

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/foundations/browser_autofill_manager.h"

#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/data_manager/addresses/address_data_manager_test_api.h"
#include "components/autofill/core/browser/data_model/credit_card.h"
#include "components/autofill/core/browser/form_import/form_data_importer_test_api.h"
#include "components/autofill/core/browser/foundations/test_autofill_client.h"
#include "components/autofill/core/browser/foundations/test_autofill_driver.h"
#include "components/autofill/core/browser/foundations/test_browser_autofill_manager.h"
#include "components/autofill/core/browser/payments/credit_card_access_manager.h"
#include "components/autofill/core/browser/payments/iban_save_manager.h"
#include "components/autofill/core/browser/payments/test_credit_card_save_manager.h"
#include "components/autofill/core/browser/payments/test_payments_autofill_client.h"
#include "components/autofill/core/browser/payments/test_payments_network_interface.h"
#include "components/autofill/core/browser/single_field_fillers/mock_single_field_fill_router.h"
#include "components/autofill/core/browser/test_utils/autofill_form_test_utils.h"
#include "components/autofill/core/browser/ui/test_autofill_external_delegate.h"
#include "components/strings/grit/components_strings.h"
#include "components/sync/test/test_sync_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

namespace autofill {
namespace {

// Copied from components/autofill/core/browser/foundations/
// browser_autofill_manager_unittest.cc

using mojom::SubmissionSource;
using test::CreateTestFormField;
using ::testing::NiceMock;
using ::testing::Return;

gfx::Rect GetFakeCaretBounds(const FormFieldData& focused_field) {
  gfx::PointF p = focused_field.bounds().origin();
  return gfx::Rect(gfx::Point(p.x(), p.y()), gfx::Size(0, 10));
}

struct TestAddressFillData {
  TestAddressFillData(const char* first,
                      const char* middle,
                      const char* last,
                      const char* address1,
                      const char* address2,
                      const char* city,
                      const char* state,
                      const char* postal_code,
                      const char* country,
                      const char* country_short,
                      const char* phone,
                      const char* email,
                      const char* company)
      : first(first),
        middle(middle),
        last(last),
        address1(address1),
        address2(address2),
        city(city),
        state(state),
        postal_code(postal_code),
        country(country),
        country_short(country_short),
        phone(phone),
        email(email),
        company(company) {}

  const char* first;
  const char* middle;
  const char* last;
  const char* address1;
  const char* address2;
  const char* city;
  const char* state;
  const char* postal_code;
  const char* country;
  const char* country_short;
  const char* phone;
  const char* email;
  const char* company;
};

TestAddressFillData GetElvisAddressFillData() {
  return {"Elvis",
          "Aaron",
          "Presley",
          "3734 Elvis Presley Blvd.",
          "Apt. 10",
          "Memphis",
          "Tennessee",
          "38116",
          "United States",
          "US",
          "2345678901",
          "theking@gmail.com",
          "RCA"};
}

// Creates a GUID for testing. For example,
// MakeGuid(123) = "00000000-0000-0000-0000-000000000123";
std::string MakeGuid(size_t last_digit) {
  return base::StringPrintf("00000000-0000-0000-0000-%012zu", last_digit);
}

class MockCreditCardAccessManager : public CreditCardAccessManager {
 public:
  using CreditCardAccessManager::CreditCardAccessManager;
  MOCK_METHOD(void, PrepareToFetchCreditCard, (), (override));
};

class MockPaymentsAutofillClient : public payments::TestPaymentsAutofillClient {
 public:
  explicit MockPaymentsAutofillClient(AutofillClient* client)
      : payments::TestPaymentsAutofillClient(client) {}
  ~MockPaymentsAutofillClient() override = default;

  MOCK_METHOD(bool, HasCreditCardScanFeature, (), (const override));
  MOCK_METHOD(void,
              OnCardDataAvailable,
              (const FilledCardInformationBubbleOptions&),
              (override));
};

class MockAutofillClient : public TestAutofillClient {
 public:
  static std::unique_ptr<MockAutofillClient> Create(
      syncer::TestSyncService* sync_service) {
    auto client = std::make_unique<NiceMock<MockAutofillClient>>();

    auto create_single_field_fill_router = [&client] {
      auto x = std::make_unique<NiceMock<MockSingleFieldFillRouter>>(
          client->GetAutocompleteHistoryManager(),
          client->GetPaymentsAutofillClient()->GetIbanManager(),
          client->GetPaymentsAutofillClient()->GetMerchantPromoCodeManager());
      // By default, if we offer single field form fill, suggestions should be
      // returned because it is assumed |field.should_autocomplete| is set to
      // true. This should be overridden in tests where
      // |field.should_autocomplete| is set to false.
      ON_CALL(*x, OnGetSingleFieldSuggestions).WillByDefault(Return(true));
      return x;
    };

    auto create_payments_autofill_client = [&client] {
      auto x = std::make_unique<MockPaymentsAutofillClient>(&*client);
      x->set_payments_network_interface(
          std::make_unique<payments::TestPaymentsNetworkInterface>(
              client->GetURLLoaderFactory(), client->GetIdentityManager(),
              &client->GetPersonalDataManager()));
      return x;
    };

    auto create_credit_card_save_manager = [&client] {
      auto x = std::make_unique<TestCreditCardSaveManager>(&*client);
      x->SetCreditCardUploadEnabled(true);
      return x;
    };

    client->set_payments_autofill_client(create_payments_autofill_client());
    client->SetPrefs(test::PrefServiceForTesting());
    client->GetPersonalDataManager().SetPrefService(client->GetPrefs());
    client->GetPersonalDataManager().SetSyncServiceForTest(sync_service);
    client->set_test_strike_database(std::make_unique<TestStrikeDatabase>());
    client->set_single_field_fill_router(create_single_field_fill_router());
    client->set_crowdsourcing_manager(
        std::make_unique<NiceMock<MockAutofillCrowdsourcingManager>>(&*client));
    test_api(client->GetPersonalDataManager().address_data_manager())
        .set_auto_accept_address_imports(true);
    test_api(*client->GetFormDataImporter())
        .set_credit_card_save_manager(create_credit_card_save_manager());
    test_api(*client->GetFormDataImporter())
        .set_iban_save_manager(std::make_unique<IbanSaveManager>(&*client));
    return client;
  }

  MockAutofillClient() = default;
  MockAutofillClient(const MockAutofillClient&) = delete;
  MockAutofillClient& operator=(const MockAutofillClient&) = delete;
  ~MockAutofillClient() override = default;

  MOCK_METHOD(profile_metrics::BrowserProfileType,
              GetProfileType,
              (),
              (const override));
  MOCK_METHOD(void,
              HideAutofillSuggestions,
              (SuggestionHidingReason reason),
              (override));
  MOCK_METHOD(void,
              TriggerUserPerceptionOfAutofillSurvey,
              (FillingProduct, (const std::map<std::string, std::string>&)),
              (override));
  MOCK_METHOD(AutofillComposeDelegate*, GetComposeDelegate, (), (override));
  MOCK_METHOD(bool,
              ShowAutofillFieldIphForFeature,
              (const FormFieldData& field, AutofillClient::IphFeature feature),
              (override));
  MOCK_METHOD(void, HideAutofillFieldIph, (), (override));
  MOCK_METHOD(void,
              ShowPlusAddressEmailOverrideNotification,
              (const std::string&, AutofillClient::EmailOverrideUndoCallback),
              (override));
};

class MockTouchToFillDelegate : public TouchToFillDelegate {
 public:
  static std::unique_ptr<MockTouchToFillDelegate> Create(
      BrowserAutofillManager* manager) {
    auto delegate = std::make_unique<NiceMock<MockTouchToFillDelegate>>();
    ON_CALL(*delegate, GetManager()).WillByDefault(Return(manager));
    ON_CALL(*delegate, IsShowingTouchToFill()).WillByDefault(Return(false));
    return delegate;
  }

  MockTouchToFillDelegate() = default;
  MockTouchToFillDelegate(const MockTouchToFillDelegate&) = delete;
  MockTouchToFillDelegate& operator=(const MockTouchToFillDelegate&) = delete;
  ~MockTouchToFillDelegate() override = default;

  MOCK_METHOD(BrowserAutofillManager*, GetManager, (), (override));
  MOCK_METHOD(bool,
              IntendsToShowTouchToFill,
              (FormGlobalId, FieldGlobalId, const FormData&),
              (override));
  MOCK_METHOD(bool,
              TryToShowTouchToFill,
              (const FormData&, const FormFieldData&),
              (override));
  MOCK_METHOD(bool, IsShowingTouchToFill, (), (override));
  MOCK_METHOD(void, HideTouchToFill, (), (override));
  MOCK_METHOD(void, Reset, (), (override));
  MOCK_METHOD(bool, ShouldShowScanCreditCard, (), (override));
  MOCK_METHOD(void, ScanCreditCard, (), (override));
  MOCK_METHOD(void, OnCreditCardScanned, (const CreditCard& card), (override));
  MOCK_METHOD(void, ShowPaymentMethodSettings, (), (override));
  MOCK_METHOD(void,
              CreditCardSuggestionSelected,
              (std::string unique_id, bool is_virtual),
              (override));
  MOCK_METHOD(void,
              IbanSuggestionSelected,
              ((absl::variant<Iban::Guid, Iban::InstrumentId>)),
              (override));
  MOCK_METHOD(void, OnDismissed, (bool dismissed_by_user), (override));
  MOCK_METHOD(void,
              LogMetricsAfterSubmission,
              (const FormStructure&),
              (override));
};

class MockAutofillDriver : public TestAutofillDriver {
 public:
  using TestAutofillDriver::TestAutofillDriver;
  MockAutofillDriver(const MockAutofillDriver&) = delete;
  MockAutofillDriver& operator=(const MockAutofillDriver&) = delete;

  // Mock methods to enable testability.
  MOCK_METHOD((base::flat_set<FieldGlobalId>),
              ApplyFormAction,
              (mojom::FormActionType action_type,
               mojom::ActionPersistence action_persistence,
               base::span<const FormFieldData> data,
               const url::Origin& triggered_origin,
               (const base::flat_map<FieldGlobalId, FieldType>&)),
              (override));
  MOCK_METHOD(void,
              ApplyFieldAction,
              (mojom::FieldActionType action_type,
               mojom::ActionPersistence action_persistence,
               const FieldGlobalId& field_id,
               const std::u16string& value),
              (override));
  MOCK_METHOD(void,
              SendTypePredictionsToRenderer,
              ((base::span<const raw_ptr<FormStructure, VectorExperimental>>)),
              (override));
};

class TestBrowserAutofillManager : public autofill::TestBrowserAutofillManager {
 public:
  static std::unique_ptr<TestBrowserAutofillManager> Create(
      AutofillDriver* driver) {
    auto manager = std::make_unique<TestBrowserAutofillManager>(driver);
    manager->set_touch_to_fill_delegate(
        MockTouchToFillDelegate::Create(&*manager));
    test_api(*manager).SetExternalDelegate(
        std::make_unique<TestAutofillExternalDelegate>(
            &*manager,
            /*call_parent_methods=*/true));
    test_api(*manager).set_credit_card_access_manager(
        std::make_unique<NiceMock<MockCreditCardAccessManager>>(
            &*manager, test_api(*manager).credit_card_form_event_logger()));
    return manager;
  }

  using autofill::TestBrowserAutofillManager::TestBrowserAutofillManager;
};

AutofillProfile FillDataToAutofillProfile(const TestAddressFillData& data) {
  AutofillProfile profile(i18n_model_definition::kLegacyHierarchyCountryCode);
  test::SetProfileInfo(&profile, data.first, data.middle, data.last, data.email,
                       data.company, data.address1, data.address2, data.city,
                       data.state, data.postal_code, data.country_short,
                       data.phone);
  return profile;
}

class BraveBrowserAutofillManagerTest : public testing::Test {
 public:
  void SetUp() override {
    client_ = CreateAutofillClient();
    driver_ = CreateAutofillDriver();
    driver_->set_autofill_manager(CreateAutofillManager());

    // Initialize the TestPersonalDataManager with some default data.
    CreateTestAutofillProfiles();
    CreateTestCreditCards();

    // Mandatory re-auth is required for credit card autofill on automotive, so
    // the authenticator response needs to be properly mocked.
#if BUILDFLAG(IS_ANDROID)
    payments_client().SetUpDeviceBiometricAuthenticatorSuccessOnAutomotive();
#endif
  }

  void TearDown() override {
    driver_.reset();
    client_.reset();
  }

  // Called by SetUp(). May be overridden by deriving fixtures.
  virtual std::unique_ptr<MockAutofillClient> CreateAutofillClient() {
    return MockAutofillClient::Create(&sync_service());
  }
  virtual std::unique_ptr<NiceMock<MockAutofillDriver>> CreateAutofillDriver() {
    return std::make_unique<NiceMock<MockAutofillDriver>>(&client());
  }
  virtual std::unique_ptr<TestBrowserAutofillManager> CreateAutofillManager() {
    return TestBrowserAutofillManager::Create(&driver());
  }

  void OnAskForValuesToFill(
      const FormData& form,
      const FormFieldData& field,
      AutofillSuggestionTriggerSource trigger_source =
          AutofillSuggestionTriggerSource::kTextFieldValueChanged) {
    manager().OnAskForValuesToFill(form, field.global_id(),
                                   GetFakeCaretBounds(field), trigger_source);
  }

  void DidShowSuggestions(const FormData& form,
                          size_t field_index = 0,
                          SuggestionType type = SuggestionType::kAddressEntry) {
    manager().DidShowSuggestions({type}, form,
                                 form.fields()[field_index].global_id());
  }

  void FormsSeen(const std::vector<FormData>& forms) {
    manager().OnFormsSeen(/*updated_forms=*/forms,
                          /*removed_forms=*/{});
  }

  void FormSubmitted(const FormData& form) {
    manager().OnFormSubmitted(form, SubmissionSource::FORM_SUBMISSION);
  }

  syncer::TestSyncService& sync_service() { return sync_service_; }

  MockAutofillClient& client() { return *client_; }

  FormDataImporter& form_data_importer() {
    return *client().GetFormDataImporter();
  }

  MockSingleFieldFillRouter& single_field_fill_router() {
    return static_cast<MockSingleFieldFillRouter&>(
        client().GetSingleFieldFillRouter());
  }

  MockPaymentsAutofillClient& payments_client() {
    return static_cast<MockPaymentsAutofillClient&>(
        *client().GetPaymentsAutofillClient());
  }

  TestPersonalDataManager& personal_data() {
    return client().GetPersonalDataManager();
  }

  MockAutofillCrowdsourcingManager& crowdsourcing_manager() {
    return static_cast<MockAutofillCrowdsourcingManager&>(
        client().GetCrowdsourcingManager());
  }

  MockAutofillDriver& driver() { return *driver_; }

  TestBrowserAutofillManager& manager() {
    return static_cast<TestBrowserAutofillManager&>(
        driver_->GetAutofillManager());
  }

  MockTouchToFillDelegate& touch_to_fill_delegate() {
    return *static_cast<MockTouchToFillDelegate*>(
        manager().touch_to_fill_delegate());
  }

  MockCreditCardAccessManager& cc_access_manager() {
    return static_cast<MockCreditCardAccessManager&>(
        manager().GetCreditCardAccessManager());
  }

  TestAutofillExternalDelegate* external_delegate() {
    return static_cast<TestAutofillExternalDelegate*>(
        test_api(manager()).external_delegate());
  }

 private:
  void CreateTestAutofillProfiles() {
    AutofillProfile profile1 =
        FillDataToAutofillProfile(GetElvisAddressFillData());
    profile1.set_guid(MakeGuid(1));
    profile1.usage_history().set_use_date(base::Time::Now() - base::Days(2));
    personal_data().address_data_manager().AddProfile(profile1);
  }

  void CreateTestCreditCards() {
    CreditCard credit_card1;
    test::SetCreditCardInfo(&credit_card1, "Elvis Presley",
                            "4234567890123456",  // Visa
                            "04", "2999", "1");
    credit_card1.set_guid(MakeGuid(4));
    credit_card1.usage_history().set_use_count(10);
    credit_card1.usage_history().set_use_date(base::Time::Now() -
                                              base::Days(5));
    personal_data().payments_data_manager().AddCreditCard(credit_card1);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  test::AutofillUnitTestEnvironment autofill_test_environment_;
  syncer::TestSyncService sync_service_;
  std::unique_ptr<MockAutofillClient> client_;
  std::unique_ptr<MockAutofillDriver> driver_;
};

// Test that if a form is mixed content we show a warning instead of any
// suggestions.
TEST_F(BraveBrowserAutofillManagerTest, Onion_MixedForm1) {
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
TEST_F(BraveBrowserAutofillManagerTest, Onion_MixedForm2) {
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
TEST_F(BraveBrowserAutofillManagerTest, Onion_NonMixedForm) {
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

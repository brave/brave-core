// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/email_aliases/email_aliases_service.h"

#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_account/prefs.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/email_aliases/features.h"
#include "brave/components/email_aliases/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/functional/overload.h"
#include "ui/base/l10n/l10n_util.h"

namespace email_aliases {

using AuthenticationStatus = email_aliases::mojom::AuthenticationStatus;

class EmailAliasesServiceTest : public ::testing::Test {
 protected:
  EmailAliasesServiceTest() {
    feature_list_.InitWithFeatures({email_aliases::features::kEmailAliases,
                                    brave_account::features::kBraveAccount},
                                   {});
    EmailAliasesService::RegisterProfilePrefs(prefs_.registry());
    brave_account::prefs::RegisterPrefs(prefs_.registry());
  }

  void SetUp() override {
    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting();
    keyed_service_ = std::make_unique<EmailAliasesService>(
        test_url_loader_factory_.GetSafeWeakWrapper(), &prefs_,
        os_crypt_.get());

    keyed_service_->BindInterface(service_.BindNewPipeAndPassReceiver());
    observer_ = email_aliases::test::AuthStateObserver::Setup(
        keyed_service_.get(), true);
  }

  base::test::ScopedFeatureList feature_list_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  std::unique_ptr<EmailAliasesService> keyed_service_;
  mojo::Remote<mojom::EmailAliasesService> service_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<test::AuthStateObserver> observer_;
};

TEST_F(EmailAliasesServiceTest, Auth) {
  EmailAliasesAuth auth(&prefs_, test::GetEncryptor(os_crypt_.get()),
                        base::BindLambdaForTesting([&]() {}));

  auth.SetAuthEmail("test@domain.com");
  EXPECT_EQ(auth.GetAuthEmail(), "test@domain.com");

  auth.SetAuthToken("token");
  EXPECT_EQ(auth.CheckAndGetAuthToken(), "token");

  auth.SetAuthEmail({});
  EXPECT_EQ(auth.GetAuthEmail(), "");
  EXPECT_EQ(auth.CheckAndGetAuthToken(), "");

  auth.SetAuthEmail("test@domain.com");
  auth.SetAuthToken("token");

  /*
  {
    // set the same email
    prefs_.SetString(std::string_view path, std::string_view value)
    prefs_.SetDict(
        prefs::kAuth,
        base::Value::Dict()
            .Set("email", "test@domain.com")
            .Set("token", *prefs_.GetDict(prefs::kAuth).FindString("token")));
    EXPECT_EQ(auth.GetAuthEmail(), "test@domain.com");
    EXPECT_EQ(auth.CheckAndGetAuthToken(), "token");
  }

  {
    // set new email
    prefs_.SetDict(
        prefs::kAuth,
        base::Value::Dict()
            .Set("email", "new@domain.com")
            .Set("token", *prefs_.GetDict(prefs::kAuth).FindString("token")));
    EXPECT_EQ(auth.GetAuthEmail(), "new@domain.com");
    EXPECT_EQ(auth.CheckAndGetAuthToken(), "");  // token reset
  }
  {
    // token becomes invalid
    auth.SetAuthToken("token");
    EXPECT_EQ(auth.GetAuthEmail(), "new@domain.com");
    EXPECT_EQ(auth.CheckAndGetAuthToken(), "token");
    prefs_.SetDict(prefs::kAuth, base::Value::Dict()
                                     .Set("email", "new@domain.com")
                                     .Set("token", "invalid"));
    EXPECT_EQ(auth.GetAuthEmail(), "new@domain.com");
    EXPECT_EQ(auth.CheckAndGetAuthToken(), "");  // token reset
  }*/
}

class AliasObserver : public mojom::EmailAliasesServiceObserver {
 public:
  void OnAuthStateChanged(mojom::AuthStatePtr) override {}

  void OnAliasesUpdated(std::vector<mojom::AliasPtr> aliases) override {
    ++alias_updates;
    last_aliases = std::move(aliases);
  }

  bool WaitForAliasUpdateCount(size_t count) {
    return base::test::RunUntil(
        [this, count]() { return alias_updates >= count; });
  }

  void BindReceiver(
      mojo::PendingReceiver<mojom::EmailAliasesServiceObserver> pending) {
    receiver.Bind(std::move(pending));
  }

  size_t alias_update_count() const { return alias_updates; }

  const std::vector<mojom::AliasPtr>& get_last_aliases() const {
    return last_aliases;
  }

 private:
  size_t alias_updates = 0;
  std::vector<mojom::AliasPtr> last_aliases;
  mojo::Receiver<mojom::EmailAliasesServiceObserver> receiver{this};
};

class EmailAliasesAPITest : public ::testing::Test {
 public:
  void AddManageResponseFor(const std::optional<std::string>& body) {
    const GURL manage_url = EmailAliasesService::GetEmailAliasesServiceURL();
    if (body.has_value()) {
      url_loader_factory_.AddResponse(manage_url.spec(), *body,
                                      base::Contains(*body, "error")
                                          ? net::HTTP_BAD_REQUEST
                                          : net::HTTP_OK);
    } else {
      network::URLLoaderCompletionStatus completion(net::ERR_FAILED);
      url_loader_factory_.AddResponse(manage_url, /*head=*/nullptr,
                                      /*content=*/"", completion);
    }
  }

  void AddRefreshResponseFor(
      const std::optional<std::string>& refresh_body = std::nullopt) {
    const GURL manage_url = EmailAliasesService::GetEmailAliasesServiceURL();
    url_loader_factory_.AddResponse(manage_url.Resolve("?status=active").spec(),
                                    refresh_body.value_or("[]"));
  }

  template <typename T, typename InvokeFn>
  base::expected<T, std::string> InvokeAndWait(InvokeFn&& invoker) {
    base::test::TestFuture<base::expected<T, std::string>> future;
    std::forward<InvokeFn>(invoker)(future.GetCallback());
    return future.Take();
  }

  base::expected<std::string, std::string> CallGenerateAliasWith(
      const std::optional<std::string>& body) {
    AddManageResponseFor(body);
    return InvokeAndWait<std::string>(
        [this](auto cb) { service_->GenerateAlias(std::move(cb)); });
  }

  base::expected<std::monostate, std::string> CallUpdateAliasWith(
      const std::string& alias_email,
      const std::optional<std::string>& put_body,
      const std::optional<std::string>& refresh_body = std::nullopt,
      bool wait_for_update = true) {
    AddManageResponseFor(put_body);
    AddRefreshResponseFor(refresh_body);
    auto result_out =
        InvokeAndWait<std::monostate>([this, &alias_email](auto cb) {
          service_->UpdateAlias(alias_email, /*note=*/std::string("note"),
                                std::move(cb));
        });
    if (wait_for_update) {
      EXPECT_TRUE(observer_.WaitForAliasUpdateCount(1));
    }
    return result_out;
  }

  base::expected<std::monostate, std::string> CallDeleteAliasWith(
      const std::string& alias_email,
      const std::optional<std::string>& delete_body) {
    AddManageResponseFor(delete_body);
    AddRefreshResponseFor(/*refresh_body=*/std::nullopt);
    auto result_out =
        InvokeAndWait<std::monostate>([this, &alias_email](auto cb) {
          service_->DeleteAlias(alias_email, std::move(cb));
        });
    EXPECT_TRUE(observer_.WaitForAliasUpdateCount(1));
    return result_out;
  }

  void SetupAuth(bool auth = true) {
    EmailAliasesAuth settings(&prefs_, test::GetEncryptor(os_crypt_.get()));
    settings.SetAuthEmail("test@example.com");
    if (auth) {
      settings.SetAuthToken("token456");
    } else {
      settings.SetAuthEmail({});
    }
  }

 protected:
  void SetUp() override {
    EmailAliasesService::RegisterProfilePrefs(prefs_.registry());
    brave_account::prefs::RegisterPrefs(prefs_.registry());
    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting();
    SetupAuth();

    service_ = std::make_unique<EmailAliasesService>(
        url_loader_factory_.GetSafeWeakWrapper(), &prefs_, os_crypt_.get());
    email_aliases::test::AuthStateObserver::Setup(service_.get(), true);

    mojo::PendingRemote<mojom::EmailAliasesServiceObserver> remote;
    observer_.BindReceiver(remote.InitWithNewPipeAndPassReceiver());
    service_->AddObserver(std::move(remote));
  }

  base::test::ScopedFeatureList brave_account_feature_list_{
      brave_account::features::kBraveAccount};
  base::test::ScopedFeatureList feature_list_{features::kEmailAliases};
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<EmailAliasesService> service_;
  AliasObserver observer_;
};

MATCHER_P(MatchesExpected, expected_result, "") {
  if (expected_result.has_value()) {
    return arg.has_value() && arg.value() == expected_result.value();
  }

  const std::string expected_error =
      std::visit(absl::Overload{
                     [](int id) { return l10n_util::GetStringUTF8(id); },
                     [](std::string string) { return string; },
                 },
                 expected_result.error());

  return !arg.has_value() && base::Contains(arg.error(), expected_error);
}

template <typename T>
struct EmailAliasesAPIParamTestCase {
  static constexpr auto kNameGenerator = [](const auto& info) {
    return info.param.name;
  };

  const char* name;
  std::optional<std::string> body;
  base::expected<T, std::variant<int, std::string>> expected_result;
};

template <typename TestCase>
class EmailAliasesAPIParamTest : public EmailAliasesAPITest,
                                 public testing::WithParamInterface<TestCase> {
 protected:
  void RunTestCase() {
    const auto& test_case = this->GetParam();
    EXPECT_THAT(TestCase::Run(*this, test_case),
                MatchesExpected(test_case.expected_result));
  }
};

// ================ GenerateAlias (parameterized) ================

struct GenerateAliasTestCase : EmailAliasesAPIParamTestCase<std::string> {
  static auto Run(EmailAliasesAPITest& test,
                  const GenerateAliasTestCase& test_case) {
    return test.CallGenerateAliasWith(test_case.body);
  }
};

using GenerateAliasParamTest = EmailAliasesAPIParamTest<GenerateAliasTestCase>;

TEST_P(GenerateAliasParamTest, HandlesResponses) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    EmailAliasesGenerateAlias,
    GenerateAliasParamTest,
    testing::Values(
        GenerateAliasTestCase{
            {.name = "Success",
             .body =
                 R"({ "message": "created",
                      "alias": "mock-1234@bravealias.com" })",
             .expected_result = base::ok("mock-1234@bravealias.com")}},
        GenerateAliasTestCase{
            {.name = "BackendError",
             .body = R"({ "message": "alias_unavailable" })",
             .expected_result = base::unexpected("alias_unavailable")}},
        GenerateAliasTestCase{
            {.name = "NoBody",
             .body = std::nullopt,
             .expected_result = base::unexpected(
                 IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY)}},
        GenerateAliasTestCase{
            {.name = "InvalidJSON",
             .body = "not a json",
             .expected_result = base::unexpected(
                 IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY)}},
        GenerateAliasTestCase{
            {.name = "UnexpectedPayload",
             .body = R"({ "message": "ok_but_no_alias" })",
             .expected_result = base::unexpected("ok_but_no_alias")}}),
    GenerateAliasTestCase::kNameGenerator);

// ================= UpdateAlias (parameterized) =================

struct UpdateAliasTestCase : EmailAliasesAPIParamTestCase<std::monostate> {
  static auto Run(EmailAliasesAPITest& test,
                  const UpdateAliasTestCase& test_case) {
    return test.CallUpdateAliasWith("alias@example.com", test_case.body);
  }
};

using UpdateAliasParamTest = EmailAliasesAPIParamTest<UpdateAliasTestCase>;

TEST_P(UpdateAliasParamTest, HandlesResponses) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    EmailAliasesUpdateAlias,
    UpdateAliasParamTest,
    testing::Values(
        UpdateAliasTestCase{{.name = "Success",
                             .body = R"({"message": "updated"})",
                             .expected_result = std::monostate()}},
        UpdateAliasTestCase{
            {.name = "BackendError",
             .body = "{\"message\":\"backend_error\"}",
             .expected_result = base::unexpected("backend_error")}},
        UpdateAliasTestCase{
            {.name = "NonUpdatedMessage",
             .body = "{\"message\":\"not_updated\"}",
             .expected_result = base::unexpected("not_updated")}},
        UpdateAliasTestCase{
            {.name = "NoBody",
             .body = std::nullopt,
             .expected_result = base::unexpected(
                 IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY)}},
        UpdateAliasTestCase{
            {.name = "InvalidJSON",
             .body = "not a json",
             .expected_result = base::unexpected(
                 IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY)}}),
    UpdateAliasTestCase::kNameGenerator);

// ================= DeleteAlias (parameterized) =================

struct DeleteAliasTestCase : EmailAliasesAPIParamTestCase<std::monostate> {
  static auto Run(EmailAliasesAPITest& test,
                  const DeleteAliasTestCase& test_case) {
    return test.CallDeleteAliasWith("alias@example.com", test_case.body);
  }
};

using DeleteAliasParamTest = EmailAliasesAPIParamTest<DeleteAliasTestCase>;

TEST_P(DeleteAliasParamTest, HandlesResponses) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    EmailAliasesDeleteAlias,
    DeleteAliasParamTest,
    testing::Values(
        DeleteAliasTestCase{{.name = "Success",
                             .body = "{\"message\":\"deleted\"}",
                             .expected_result = std::monostate()}},
        DeleteAliasTestCase{
            {.name = "BackendError",
             .body = "{\"message\":\"backend_error\"}",
             .expected_result = base::unexpected("backend_error")}},
        DeleteAliasTestCase{
            {.name = "NoBody",
             .body = std::nullopt,
             .expected_result = base::unexpected(
                 IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY)}},
        DeleteAliasTestCase{
            {.name = "InvalidJSON",
             .body = "not a json",
             .expected_result = base::unexpected(
                 IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY)}}),
    DeleteAliasTestCase::kNameGenerator);

TEST_F(EmailAliasesAPITest, RefreshAliases_Notifies_OnValidResponse) {
  const std::string alias_email = "alias@example.com";
  auto result_out = CallUpdateAliasWith(
      alias_email,
      /*put_body=*/R"({"message":"updated"})",
      /*refresh_body=*/
      std::string("[{\"email\":\"dest@example.com\",\"alias\":\"") +
          alias_email +
          "\",\"created_at\":\"2025-01-01T00:00:00Z\",\"last_used\":\"\","
          "\"status\":\"active\"}]");
  ASSERT_TRUE(result_out.has_value());
  ASSERT_FALSE(observer_.get_last_aliases().empty());
  EXPECT_EQ(observer_.get_last_aliases()[0]->email, alias_email);
}

TEST_F(EmailAliasesAPITest, RefreshAliases_DoesNotNotify_OnErrorOrInvalidJson) {
  auto result_out =
      CallUpdateAliasWith("alias@example.com",
                          /*put_body=*/R"({"message":"updated"})",
                          /*refresh_body=*/R"({"message":"backend_error"})",
                          /*wait_for_update=*/false);
  ASSERT_TRUE(result_out.has_value());
  EXPECT_EQ(observer_.alias_update_count(), 0u);
}

TEST_F(EmailAliasesAPITest, ApiFetch_AttachesAuthTokenAndAPIKeyHeaders) {
  EmailAliasesAuth auth(&prefs_, test::GetEncryptor(os_crypt_.get()),
                        base::BindLambdaForTesting([&]() {}));
  auth.SetAuthEmail("test@domain.com");
  auth.SetAuthToken("auth456");

  // Wait until auth token is set by the session poll response.
  EXPECT_TRUE(base::test::RunUntil(
      [&]() { return service_->GetAuthTokenForTesting() == "auth456"; }));

  // Intercept the next manage request to capture headers.
  const GURL manage_url = EmailAliasesService::GetEmailAliasesServiceURL();
  std::string seen_authorization;
  std::string seen_api_key;
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        if (request.url == manage_url) {
          if (auto v = request.headers.GetHeader("Authorization")) {
            seen_authorization = *v;
          }
          if (auto v = request.headers.GetHeader("X-API-key")) {
            seen_api_key = *v;
          }
        }
      }));

  // Provide a successful GenerateAlias response to trigger ApiFetch.
  const std::string alias_email = "mock-1234@bravealias.com";
  url_loader_factory_.AddResponse(
      manage_url.spec(),
      std::string(R"({"message":"created","alias":"")") + alias_email + "\"}");

  auto gen_result = CallGenerateAliasWith(std::nullopt);
  // The helper enqueues the request body separately; here we just ensure it
  // ran. Validate headers captured by the interceptor.
  EXPECT_EQ(seen_authorization, "Bearer auth456");
  EXPECT_EQ(seen_api_key, BUILDFLAG(BRAVE_SERVICES_KEY));
}

}  // namespace email_aliases

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/email_aliases/email_aliases_service.h"

#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_account/mock_brave_account_authentication.h"
#include "brave/components/brave_account/prefs.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/constants/network_constants.h"
#include "brave/components/email_aliases/features.h"
#include "brave/components/email_aliases/mock_endpoint.h"
#include "brave/components/email_aliases/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace email_aliases {

using ::testing::_;

using AuthenticationStatus = email_aliases::mojom::AuthenticationStatus;

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
  template <typename T, typename InvokeFn>
  base::expected<T, std::string> InvokeAndWait(InvokeFn&& invoker) {
    base::test::TestFuture<base::expected<T, std::string>> future;
    std::forward<InvokeFn>(invoker)(future.GetCallback());
    return future.Take();
  }

  base::expected<std::string, std::string> CallGenerateAliasWith() {
    return InvokeAndWait<std::string>(
        [this](auto cb) { service_->GenerateAlias(std::move(cb)); });
  }

  base::expected<std::monostate, std::string> CallUpdateAliasWith(
      const std::string& alias_email) {
    auto result_out =
        InvokeAndWait<std::monostate>([this, &alias_email](auto cb) {
          service_->UpdateAlias(alias_email, /*note=*/std::string("note"),
                                std::move(cb));
        });
    return result_out;
  }

  base::expected<std::monostate, std::string> CallDeleteAliasWith(
      const std::string& alias_email) {
    auto result_out =
        InvokeAndWait<std::monostate>([this, &alias_email](auto cb) {
          service_->DeleteAlias(alias_email, std::move(cb));
        });
    return result_out;
  }

 protected:
  void SetUp() override {
    EmailAliasesService::RegisterProfilePrefs(prefs_.registry());
    brave_account::prefs::RegisterPrefs(prefs_.registry());

    brave_account_auth_ = std::make_unique<
        testing::NiceMock<brave_account::MockBraveAccountAuthentication>>();
    ON_CALL(*brave_account_auth_,
            GetServiceToken(brave_account::mojom::Service::kEmailAliases, _))
        .WillByDefault([](auto service, auto callback) {
          auto token = brave_account::mojom::GetServiceTokenResult::New();
          token->serviceToken = "email-aliases-token";
          std::move(callback).Run(base::ok(std::move(token)));
        });

    service_ = std::make_unique<EmailAliasesService>(
        brave_account_auth_->BindAndGetRemote(),
        url_loader_factory_.GetSafeWeakWrapper(), &prefs_);
    email_aliases::test::AuthStateObserver::Setup(service_.get(), true);
    service_->GetAuth()->SetAuthEmailForTesting("test@login.com");

    mojo::PendingRemote<mojom::EmailAliasesServiceObserver> remote;
    observer_.BindReceiver(remote.InitWithNewPipeAndPassReceiver());
    service_->AddObserver(std::move(remote));
  }

  template <typename... Args>
  std::string GetErrorMessage(int idc, Args... args) {
    if constexpr (sizeof...(Args) == 0) {
      return l10n_util::GetStringUTF8(idc);
    } else {
      return l10n_util::GetStringFUTF8(idc, base::UTF8ToUTF16(args)...);
    }
  }

  base::test::ScopedFeatureList brave_account_feature_list_{
      brave_account::features::BraveAccountFeatureForTesting()};
  base::test::ScopedFeatureList feature_list_{features::kEmailAliases};
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<
      testing::NiceMock<brave_account::MockBraveAccountAuthentication>>
      brave_account_auth_;
  std::unique_ptr<EmailAliasesService> service_;
  AliasObserver observer_;
};

TEST_F(EmailAliasesAPITest, RefreshAliases_Notifies_OnValidResponse) {
  const std::string alias_email = "alias@example.com";

  test::MockResponseFor<endpoints::UpdateAlias>(url_loader_factory_, []() {
    endpoints::UpdateAlias::Response::SuccessBody r;
    r.message = "updated";
    return r;
  });
  test::MockResponseFor<endpoints::AliasList>(
      url_loader_factory_, [&alias_email]() {
        email_aliases::AliasListEntry entry;
        entry.alias = alias_email;
        entry.email = "dest@example.com";
        entry.status = "active";
        entry.created_at = "2025-01-01T00:00:00Z";
        endpoints::AliasList::Response::SuccessBody r;
        r.result.push_back(std::move(entry));
        return r;
      });

  auto result_out = CallUpdateAliasWith(alias_email);
  EXPECT_TRUE(observer_.WaitForAliasUpdateCount(1));
  ASSERT_FALSE(observer_.get_last_aliases().empty());
  EXPECT_EQ(observer_.get_last_aliases()[0]->email, alias_email);
}

TEST_F(EmailAliasesAPITest, RefreshAliases_DoesNotNotify_OnErrorOrInvalidJson) {
  test::MockResponseFor<endpoints::UpdateAlias>(url_loader_factory_, []() {
    endpoints::UpdateAlias::Response::SuccessBody r;
    r.message = "updated";
    return r;
  });
  test::MockResponseFor<endpoints::AliasList>(url_loader_factory_, []() {
    endpoints::AliasList::Response::ErrorBody r;
    r.message = "backend_error";
    return r;
  });
  auto result_out = CallUpdateAliasWith("alias@example.com");
  ASSERT_TRUE(result_out.has_value());
  EXPECT_EQ(observer_.alias_update_count(), 0u);
}

TEST_F(EmailAliasesAPITest, ApiFetch_AttachesAuthTokenAndAPIKeyHeaders) {
  // Intercept the next manage request to capture headers.
  const GURL manage_url = test::GetEmailAliasesServiceURL();
  std::string seen_authorization;
  std::string seen_api_key;
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        if (request.url == manage_url) {
          if (auto v = request.headers.GetHeader("Authorization")) {
            seen_authorization = *v;
          }
          if (auto v = request.headers.GetHeader(kBraveServicesKeyHeader)) {
            seen_api_key = *v;
          }
        }
      }));

  // Provide a successful GenerateAlias response to trigger ApiFetch.
  const std::string alias_email = "mock-1234@bravealias.com";
  test::MockResponseFor<endpoints::GenerateAlias>(
      url_loader_factory_, [&alias_email]() {
        endpoints::GenerateAlias::Response::SuccessBody r;
        r.message = "created";
        r.alias = alias_email;
        return r;
      });

  auto gen_result = CallGenerateAliasWith();
  // The helper enqueues the request body separately; here we just ensure it
  // ran. Validate headers captured by the interceptor.
  EXPECT_EQ(seen_authorization, "Bearer email-aliases-token");
  EXPECT_EQ(seen_api_key, BUILDFLAG(BRAVE_SERVICES_KEY));
}

TEST_F(EmailAliasesAPITest, GenerateAlias) {
  {
    test::MockResponseFor<endpoints::GenerateAlias>(url_loader_factory_, []() {
      endpoints::GenerateAlias::Response::SuccessBody r;
      r.message = "created";
      r.alias = "test@alias";
      return r;
    });
    auto result = CallGenerateAliasWith();
    EXPECT_EQ("test@alias", result);
  }
  {
    test::MockResponseFor<endpoints::GenerateAlias>(url_loader_factory_, []() {
      endpoints::GenerateAlias::Response::SuccessBody r;
      r.message = "???";
      r.alias = "test@alias";
      return r;
    });
    auto result = CallGenerateAliasWith();
    EXPECT_EQ(
        GetErrorMessage(IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY),
        result.error());
  }
  {
    test::MockResponseFor<endpoints::GenerateAlias>(url_loader_factory_, []() {
      endpoints::GenerateAlias::Response::ErrorBody r;
      r.message = "backend_error";
      return r;
    });
    auto result = CallGenerateAliasWith();
    EXPECT_EQ(GetErrorMessage(IDS_EMAIL_ALIASES_SERVICE_REPORTED_ERROR,
                              "backend_error"),
              result.error());
  }
  {
    test::MockResponseFor<endpoints::GenerateAlias>(url_loader_factory_, "");
    auto result = CallGenerateAliasWith();
    EXPECT_EQ(GetErrorMessage(IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY),
              result.error());
  }
}

TEST_F(EmailAliasesAPITest, UpdateAlias) {
  {
    test::MockResponseFor<endpoints::UpdateAlias>(url_loader_factory_, []() {
      endpoints::UpdateAlias::Response::SuccessBody r;
      r.message = "updated";
      return r;
    });
    auto result = CallUpdateAliasWith("test");
    EXPECT_EQ(std::monostate{}, result);
  }
  {
    test::MockResponseFor<endpoints::UpdateAlias>(url_loader_factory_, []() {
      endpoints::UpdateAlias::Response::SuccessBody r;
      r.message = "???";
      return r;
    });
    auto result = CallUpdateAliasWith("test");
    EXPECT_EQ(
        GetErrorMessage(IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY),
        result.error());
  }
  {
    test::MockResponseFor<endpoints::UpdateAlias>(url_loader_factory_, []() {
      endpoints::GenerateAlias::Response::ErrorBody r;
      r.message = "backend_error";
      return r;
    });
    auto result = CallUpdateAliasWith("test");
    EXPECT_EQ(GetErrorMessage(IDS_EMAIL_ALIASES_SERVICE_REPORTED_ERROR,
                              "backend_error"),
              result.error());
  }
  {
    test::MockResponseFor<endpoints::UpdateAlias>(url_loader_factory_, "");
    auto result = CallUpdateAliasWith("test");
    EXPECT_EQ(GetErrorMessage(IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY),
              result.error());
  }
}

TEST_F(EmailAliasesAPITest, DeleteAlias) {
  {
    test::MockResponseFor<endpoints::DeleteAlias>(url_loader_factory_, []() {
      endpoints::DeleteAlias::Response::SuccessBody r;
      r.message = "deleted";
      return r;
    });
    auto result = CallDeleteAliasWith("test");
    EXPECT_EQ(std::monostate{}, result);
  }
  {
    test::MockResponseFor<endpoints::DeleteAlias>(url_loader_factory_, []() {
      endpoints::DeleteAlias::Response::SuccessBody r;
      r.message = "???";
      return r;
    });
    auto result = CallDeleteAliasWith("test");
    EXPECT_EQ(
        GetErrorMessage(IDS_EMAIL_ALIASES_SERVICE_ERROR_INVALID_RESPONSE_BODY),
        result.error());
  }
  {
    test::MockResponseFor<endpoints::DeleteAlias>(url_loader_factory_, []() {
      endpoints::DeleteAlias::Response::ErrorBody r;
      r.message = "backend_error";
      return r;
    });
    auto result = CallDeleteAliasWith("test");
    EXPECT_EQ(GetErrorMessage(IDS_EMAIL_ALIASES_SERVICE_REPORTED_ERROR,
                              "backend_error"),
              result.error());
  }
  {
    test::MockResponseFor<endpoints::DeleteAlias>(url_loader_factory_, "");
    auto result = CallDeleteAliasWith("test");
    EXPECT_EQ(GetErrorMessage(IDS_EMAIL_ALIASES_SERVICE_ERROR_NO_RESPONSE_BODY),
              result.error());
  }
}

}  // namespace email_aliases

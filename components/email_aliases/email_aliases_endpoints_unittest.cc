// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/email_aliases/email_aliases_endpoints.h"

#include <optional>
#include <string>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/email_aliases/mock_endpoint.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace email_aliases {

template <typename T, typename E>
bool operator==(const brave_account::endpoint_client::Response<T, E>& lhs,
                const brave_account::endpoint_client::Response<T, E>& rhs) {
  return lhs.net_error == rhs.net_error && lhs.status_code == rhs.status_code &&
         lhs.body == rhs.body;
}

template <typename Endpoint>
struct TestCase {
  std::string name;
  typename Endpoint::Response response;
  typename Endpoint::Response expected_response;
  std::optional<std::string> raw_response = std::nullopt;
};

template <typename Endpoint>
class EndpointTestBase : public ::testing::Test {
 public:
  void RunTest(TestCase<Endpoint> test_case) {
    SCOPED_TRACE(test_case.name);
    if (test_case.raw_response) {
      test::MockResponseFor<Endpoint>(
          test_url_loader_factory_, test_case.raw_response.value(),
          test_case.response.status_code, test_case.response.net_error);
    } else {
      test::MockResponseFor<Endpoint>(test_url_loader_factory_,
                                      std::move(test_case.response));
    }
    base::test::TestFuture<typename Endpoint::Response> future;
    brave_account::endpoint_client::Client<Endpoint>::Send(
        test_url_loader_factory_.GetSafeWeakWrapper(),
        typename Endpoint::Request{}, future.GetCallback());
    EXPECT_EQ(future.Take(), test_case.expected_response);
  }

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

// ================ GenerateAlias ================

bool operator==(const endpoints::GenerateAlias::Response::SuccessBody& lhs,
                const endpoints::GenerateAlias::Response::SuccessBody& rhs) {
  return lhs.message == rhs.message && lhs.alias == rhs.alias;
}

bool operator==(const endpoints::GenerateAlias::Response::ErrorBody& lhs,
                const endpoints::GenerateAlias::Response::ErrorBody& rhs) {
  return lhs.message == rhs.message;
}

namespace generate {

endpoints::GenerateAlias::Response Success() {
  endpoints::GenerateAlias::Response::SuccessBody r;
  r.message = "created";
  r.alias = "mock-1234@bravealias.com";
  return {
      .net_error = net::OK, .status_code = net::HTTP_OK, .body = std::move(r)};
}

endpoints::GenerateAlias::Response UnavailableError() {
  endpoints::GenerateAlias::Response::ErrorBody r;
  r.message = "alias_unavailable_error";
  return {.net_error = net::OK,
          .status_code = net::HTTP_BAD_REQUEST,
          .body = base::unexpected(std::move(r))};
}

endpoints::GenerateAlias::Response InvalidBody() {
  return {
      .net_error = net::OK, .status_code = net::HTTP_OK, .body = std::nullopt};
}

endpoints::GenerateAlias::Response UnexpectedPayload() {
  return {
      .net_error = net::OK, .status_code = net::HTTP_OK, .body = std::nullopt};
}

using GenerateAliasTestCase = TestCase<endpoints::GenerateAlias>;
using GenerateAliasTest = EndpointTestBase<endpoints::GenerateAlias>;

TEST_F(GenerateAliasTest, Check) {
  RunTest(GenerateAliasTestCase{
      .name = "Success",
      .response = Success(),
      .expected_response = Success(),
  });

  RunTest(GenerateAliasTestCase{
      .name = "BackendError",
      .response = UnavailableError(),
      .expected_response = UnavailableError(),
  });

  RunTest(GenerateAliasTestCase{
      .name = "InvalidBody",
      .response = InvalidBody(),
      .expected_response = InvalidBody(),
      .raw_response = "InvalidBody",
  });

  RunTest(GenerateAliasTestCase{
      .name = "UnexpectedPayload",
      .response = UnexpectedPayload(),
      .expected_response = UnexpectedPayload(),
      .raw_response = R"({ "message": "ok_but_no_alias" })",
  });
}

}  // namespace generate

// ================= UpdateAlias =================

bool operator==(const endpoints::UpdateAlias::Response::SuccessBody& lhs,
                const endpoints::UpdateAlias::Response::SuccessBody& rhs) {
  return lhs.message == rhs.message;
}

namespace update {

endpoints::UpdateAlias::Response Success() {
  endpoints::UpdateAlias::Response::SuccessBody r;
  r.message = "updated";
  return {
      .net_error = net::OK, .status_code = net::HTTP_OK, .body = std::move(r)};
}

endpoints::UpdateAlias::Response UnavailableError() {
  endpoints::UpdateAlias::Response::ErrorBody r;
  r.message = "backend_error";
  return {.net_error = net::OK,
          .status_code = net::HTTP_BAD_REQUEST,
          .body = base::unexpected(std::move(r))};
}

endpoints::UpdateAlias::Response NonUpdatedMessage() {
  endpoints::UpdateAlias::Response::SuccessBody r;
  r.message = "not_updated";
  return {
      .net_error = net::OK, .status_code = net::HTTP_OK, .body = std::move(r)};
}

endpoints::UpdateAlias::Response InvalidBody() {
  return {
      .net_error = net::OK, .status_code = net::HTTP_OK, .body = std::nullopt};
}

using UpdateAliasTestCase = TestCase<endpoints::UpdateAlias>;
using UpdateAliasTest = EndpointTestBase<endpoints::UpdateAlias>;

TEST_F(UpdateAliasTest, Check) {
  RunTest(UpdateAliasTestCase{
      .name = "Success",
      .response = Success(),
      .expected_response = Success(),
  });

  RunTest(UpdateAliasTestCase{
      .name = "BackendError",
      .response = UnavailableError(),
      .expected_response = UnavailableError(),
  });

  RunTest(UpdateAliasTestCase{.name = "NonUpdatedMessage",
                              .response = NonUpdatedMessage(),
                              .expected_response = NonUpdatedMessage()});

  RunTest(UpdateAliasTestCase{
      .name = "InvalidBody",
      .response = InvalidBody(),
      .expected_response = InvalidBody(),
      .raw_response = "InvalidBody",
  });
}

}  // namespace update

// ================= DeleteAlias =================

namespace delete_ep {

endpoints::DeleteAlias::Response Success() {
  endpoints::DeleteAlias::Response::SuccessBody r;
  r.message = "deleted";
  return {
      .net_error = net::OK, .status_code = net::HTTP_OK, .body = std::move(r)};
}

endpoints::DeleteAlias::Response UnavailableError() {
  endpoints::DeleteAlias::Response::ErrorBody r;
  r.message = "backend_error";
  return {.net_error = net::OK,
          .status_code = net::HTTP_BAD_REQUEST,
          .body = base::unexpected(std::move(r))};
}

endpoints::DeleteAlias::Response NonUpdatedMessage() {
  endpoints::DeleteAlias::Response::SuccessBody r;
  r.message = "not_deleted";
  return {
      .net_error = net::OK, .status_code = net::HTTP_OK, .body = std::move(r)};
}

endpoints::DeleteAlias::Response InvalidBody() {
  return {
      .net_error = net::OK, .status_code = net::HTTP_OK, .body = std::nullopt};
}

using DeleteAliasTestCase = TestCase<endpoints::DeleteAlias>;
using DeleteAliasTest = EndpointTestBase<endpoints::DeleteAlias>;

TEST_F(DeleteAliasTest, Check) {
  RunTest(DeleteAliasTestCase{
      .name = "Success",
      .response = Success(),
      .expected_response = Success(),
  });

  RunTest(DeleteAliasTestCase{
      .name = "BackendError",
      .response = UnavailableError(),
      .expected_response = UnavailableError(),
  });

  RunTest(DeleteAliasTestCase{.name = "NonUpdatedMessage",
                              .response = NonUpdatedMessage(),
                              .expected_response = NonUpdatedMessage()});

  RunTest(DeleteAliasTestCase{
      .name = "InvalidBody",
      .response = InvalidBody(),
      .expected_response = InvalidBody(),
      .raw_response = "InvalidBody",
  });
}

}  // namespace delete_ep

}  // namespace email_aliases

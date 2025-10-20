/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_FUNCTIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_FUNCTIONS_H_

#include <type_traits>
#include <variant>

#include "brave/components/brave_account/endpoint_client/concepts.h"

namespace brave_account::endpoint_client::functions::detail {

template <typename Handler, typename T>
concept CanHandle = requires(Handler&& h, T&& value) {
  std::forward<Handler>(h)(std::forward<T>(value));
};

template <typename ErrorReturnType, typename Errors, typename... ErrorHandlers>
constexpr auto HandleErrors(Errors&& errors,
                            ErrorHandlers&&... on_error) noexcept {
  bool handled = false;

  return std::visit(
      [&](auto&& value) -> ErrorReturnType {
        using ErrorType = decltype(value);
        ErrorReturnType result;
        (
            [&] {
              if constexpr (CanHandle<ErrorHandlers, ErrorType>) {
                if (!handled) {
                  auto invoke_on_error = [&]() -> decltype(auto) {
                    return std::invoke(std::forward<ErrorHandlers>(on_error),
                                       std::forward<ErrorType>(value));
                  };
                  static_assert(
                      concepts::ErrorHandler<decltype(invoke_on_error),
                                             ErrorReturnType>,
                      "Error handler should return a value which is "
                      "convertible to |on_reposnse| error type.");

                  result = ErrorReturnType(std::invoke(invoke_on_error));
                  handled = true;
                }
              }
            }(),
            ...);

        return result;
      },
      std::move(errors));
}

}  // namespace brave_account::endpoint_client::functions::detail

namespace brave_account::endpoint_client::functions {

//  This function provides a unified way to handle expected-type replies where
//  you want to:
//  - Process successful responses with a response handler
//  - Handle different error types with multiple error handlers
//
//  |Reply| The reply type (must have value() and has_value() methods
//  |ResponseHandler| Callable that handles successful responses, must
//  return base::expected<T, E>
//  |ErrorHandlers...| Error handlers for different error types. If there is no
//  suitable handler for an error then E() is returned. All error handlers
//  should return value which is convertible to E.
//
//  base::expected<ValueType, ErrorType> where:
//    - ValueType and ErrorType comes from on_response's return type
//    - ErrorType is unified from all error handlers
//
//  Example usage:
//  base::expected<Response, Error> reply = Client::Send(...);
//
//  auto result = HandleReply(
//    std::move(reply),
//    [&](Response response) -> base::expected<Data, ProcessError> {
//      return ProcessResponse(response);
//    },
//    [&](EndpointError error) -> ProcessError {
//      return ProcessError::Failure;
//    },
//    [&](NetworkError error) -> ProcessError {
//      return ProcessError::Network;
//    },
//    [&](ParseErrpr error) -> ProcessError {
//      return ProcessError::Parse;
//    }
//  );
//
//  Example usage:
//  auto result = HandleReply(
//    std::move(reply),
//    [&](Response response) -> base::expected<Data, ProcessError> {
//      return ProcessResponse(response);
//    },
//    [&](EndpointError error) -> ProcessError {
//      return ProcessError::Failure;
//    },
//    /* If you don't care about specific error type use this signature, this
//    should be the last error handler */
//    [&](auto error) -> ProcessError {
//      return ProcessError::GenericFailure;
//    }
//  );

template <typename Reply,
          concepts::ResponseHandler<typename Reply::value_type> ResponseHandler,
          typename... ErrorHandlers>
constexpr auto HandleReply(Reply&& reply,
                           ResponseHandler&& on_response,
                           ErrorHandlers&&... on_error) noexcept {
  auto invoke_on_response = [&]() -> decltype(auto) {
    return std::invoke(std::forward<ResponseHandler>(on_response),
                       std::forward<Reply>(reply).value());
  };

  using ResultType = decltype(invoke_on_response());
  static_assert(concepts::IsExpected<ResultType>,
                "|on_response| should return base::expected<T,E>");
  using ErrorType = typename ResultType::error_type;

  if (!reply.has_value()) {
    return ResultType(base::unexpect,
                      detail::HandleErrors<ErrorType>(
                          std::move(reply).error(),
                          std::forward<ErrorHandlers>(on_error)...));
  }
  return invoke_on_response();
}

}  // namespace brave_account::endpoint_client::functions

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_FUNCTIONS_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_BUILDER_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_BUILDER_H_

#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/json/json_writer.h"
#include "base/types/always_false.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/http/http_request_headers.h"

// See //brave/components/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace detail {
template <typename T>
concept ToValue = requires(T t) { base::WriteJson(t.ToValue()); } &&
                  std::is_member_function_pointer_v<decltype(&T::ToValue)>;

template <typename T>
struct FromValueImpl {
  static constexpr bool value =
      requires(api_request_helper::APIRequestResult result) {
        { T::FromValue(result.value_body()) } -> std::same_as<std::optional<T>>;
      };
};

// Please use `using Response/Error = T;` instead of std::variant<T>.
template <typename T>
struct FromValueImpl<std::variant<T>> {
  static constexpr bool value = false;
};

template <typename... Ts>
struct FromValueImpl<std::variant<Ts...>> {
  static constexpr bool value = (FromValueImpl<Ts>::value && ...);
};

template <typename T>
concept FromValue = FromValueImpl<T>::value;

template <typename T>
concept URL = requires {
  { T::URL() } -> std::same_as<GURL>;
};

template <auto>
inline constexpr bool dependent_false_v = false;
}  // namespace detail

namespace endpoints {
namespace concepts {
template <typename T>
concept Request = detail::ToValue<T>;

template <typename T>
concept Response = detail::FromValue<T>;

template <typename T>
concept Error = detail::FromValue<T>;

template <typename T>
concept Endpoint = detail::URL<T>;
}  // namespace concepts

enum class HTTPMethod {
  kConnect,
  kDelete,
  kGet,
  kHead,
  kOptions,
  kPatch,
  kPost,
  kPut,
  kTrace,
  kTrack
};

template <concepts::Request R, HTTPMethod M>
struct WithMethod : R {
  using Request = R;

  static constexpr std::string_view Method() {
    if constexpr (M == HTTPMethod::kConnect) {
      return net::HttpRequestHeaders::kConnectMethod;
    } else if constexpr (M == HTTPMethod::kDelete) {
      return net::HttpRequestHeaders::kDeleteMethod;
    } else if constexpr (M == HTTPMethod::kGet) {
      return net::HttpRequestHeaders::kGetMethod;
    } else if constexpr (M == HTTPMethod::kHead) {
      return net::HttpRequestHeaders::kHeadMethod;
    } else if constexpr (M == HTTPMethod::kOptions) {
      return net::HttpRequestHeaders::kOptionsMethod;
    } else if constexpr (M == HTTPMethod::kPatch) {
      return net::HttpRequestHeaders::kPatchMethod;
    } else if constexpr (M == HTTPMethod::kPost) {
      return net::HttpRequestHeaders::kPostMethod;
    } else if constexpr (M == HTTPMethod::kPut) {
      return net::HttpRequestHeaders::kPutMethod;
    } else if constexpr (M == HTTPMethod::kTrace) {
      return net::HttpRequestHeaders::kTraceMethod;
    } else if constexpr (M == HTTPMethod::kTrack) {
      return net::HttpRequestHeaders::kTrackMethod;
    } else {
      static_assert(detail::dependent_false_v<M>,
                    "Unhandled HTTP method enum!");
    }
  }

  net::HttpRequestHeaders headers;
};

template <typename>
struct is_with_method : std::false_type {};

template <typename R, HTTPMethod M>
struct is_with_method<WithMethod<R, M>> : std::true_type {};

template <typename R>
inline constexpr bool is_with_method_v = is_with_method<R>::value;

namespace concepts {
template <typename T>
concept RequestWithMethod = is_with_method_v<T> && Request<typename T::Request>;
}  // namespace concepts

template <concepts::Request R>
using CONNECT = WithMethod<R, HTTPMethod::kConnect>;

template <concepts::Request R>
using DELETE = WithMethod<R, HTTPMethod::kDelete>;

template <concepts::Request R>
using GET = WithMethod<R, HTTPMethod::kGet>;

template <concepts::Request R>
using HEAD = WithMethod<R, HTTPMethod::kHead>;

template <concepts::Request R>
using OPTIONS = WithMethod<R, HTTPMethod::kOptions>;

template <concepts::Request R>
using PATCH = WithMethod<R, HTTPMethod::kPatch>;

template <concepts::Request R>
using POST = WithMethod<R, HTTPMethod::kPost>;

template <concepts::Request R>
using PUT = WithMethod<R, HTTPMethod::kPut>;

template <concepts::Request R>
using TRACE = WithMethod<R, HTTPMethod::kTrace>;

template <concepts::Request R>
using TRACK = WithMethod<R, HTTPMethod::kTrack>;

template <concepts::RequestWithMethod Req,
          concepts::Response Resp,
          concepts::Error Err>
struct Row {
  using Request = Req;
  using Response = Resp;
  using Error = Err;
};

template <concepts::RequestWithMethod Request>
struct ForRequest {
  template <concepts::Response Response, concepts::Response... Responses>
  struct RespondsWith {
    template <concepts::Error Error, concepts::Error... Errors>
    using ErrorsWith =
        Row<Request,
            std::conditional_t<sizeof...(Responses) == 0,
                               Response,
                               std::variant<Response, Responses...>>,
            std::conditional_t<sizeof...(Errors) == 0,
                               Error,
                               std::variant<Error, Errors...>>>;
  };
};

template <typename...>
struct unique_types : std::true_type {};

template <typename T, typename... Ts>
struct unique_types<T, Ts...>
    : std::bool_constant<(!std::is_same_v<T, Ts> && ...) &&
                         unique_types<Ts...>::value> {};

template <typename... Rows>
struct Endpoint {
  template <class T>
  static constexpr bool CompletedRow = requires {
    typename T::Request;
    typename T::Response;
    typename T::Error;
  };
  static_assert((CompletedRow<Rows> && ...),
                "Each ForRequest<R> must be followed by "
                "RespondsWith<...>::ErrorsWith<...>");
  static_assert(unique_types<typename Rows::Request...>::value,
                "Duplicate ForRequest<...>: each Request type may appear at "
                "most once in an Endpoint.");

  using Request = std::variant<typename Rows::Request...>;

  template <typename Tagged>
  static constexpr bool accepts_v =
      (std::is_same_v<Tagged, typename Rows::Request> || ...);

  template <class Tagged, class R0, class... Rest>
  struct Find {
    using type =
        std::conditional_t<std::is_same_v<Tagged, typename R0::Request>,
                           R0,
                           typename Find<Tagged, Rest...>::type>;
  };

  template <class Tagged, class R0>
  struct Find<Tagged, R0> {
    using type = std::
        conditional_t<std::is_same_v<Tagged, typename R0::Request>, R0, void>;
  };

  template <concepts::RequestWithMethod R>
  using RowFor = typename Find<R, Rows...>::type;

  template <concepts::RequestWithMethod R>
  using ResponseFor = typename RowFor<R>::Response;

  template <concepts::RequestWithMethod R>
  using ErrorFor = typename RowFor<R>::Error;

  template <concepts::RequestWithMethod R>
  using ExpectedFor =
      base::expected<std::optional<ResponseFor<R>>, std::optional<ErrorFor<R>>>;

  template <concepts::RequestWithMethod R>
  using CallbackFor = base::OnceCallback<void(int, ExpectedFor<R>)>;
};

namespace concepts {
template <typename R, typename Ep>
concept Accepts = Ep::template accepts_v<R>;
}  // namespace concepts

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_BUILDER_H_

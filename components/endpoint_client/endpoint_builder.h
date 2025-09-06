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

namespace endpoints::concepts {

namespace detail {
template <typename T>
concept ToValue = requires(T t) { base::WriteJson(t.ToValue()); } &&
                  std::is_member_function_pointer_v<decltype(&T::ToValue)>;

template <typename T>
concept FromValue = requires(api_request_helper::APIRequestResult result) {
  { T::FromValue(result.value_body()) } -> std::same_as<std::optional<T>>;
};

template <typename T>
concept URL = requires {
  { T::URL() } -> std::same_as<GURL>;
};

template <typename T>
concept Method = requires {
  { T::Method() } -> std::same_as<std::string_view>;
};

template <typename T>
concept Headers = requires(T t) {
  { t.headers } -> std::same_as<net::HttpRequestHeaders&>;
};

}  // namespace detail

template <typename T>
concept RequestBody = detail::ToValue<T>;

template <typename T>
concept Request = RequestBody<T> && detail::Method<T> && detail::Headers<T>;

template <typename T>
concept Response = detail::FromValue<T>;

template <typename T>
concept Error = detail::FromValue<T>;

template <typename T>
concept Endpoint = detail::URL<T>;

}  // namespace endpoints::concepts

namespace endpoints::detail {
template <typename, typename, typename>
struct Entry;
}  // namespace endpoints::detail

namespace endpoints::concepts::detail {
template <typename T>
struct is_entry_specialization : std::false_type {};

template <typename Req, typename Resp, typename Err>
struct is_entry_specialization<endpoints::detail::Entry<Req, Resp, Err>>
    : std::true_type {};

template <typename T>
concept Entry = is_entry_specialization<std::remove_cvref_t<T>>::value;
}  // namespace endpoints::concepts::detail

namespace detail {
template <auto...>
inline constexpr bool dependent_false_v = false;
}  // namespace detail

namespace endpoints {

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

namespace detail {
template <concepts::RequestBody RB, HTTPMethod M>
struct Request : RB {
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
      static_assert(::detail::dependent_false_v<M>,
                    "Unhandled HTTP method enum!");
    }
  }

  net::HttpRequestHeaders headers;
};
}  // namespace detail

template <concepts::RequestBody R>
using CONNECT = detail::Request<R, HTTPMethod::kConnect>;

template <concepts::RequestBody R>
using DELETE = detail::Request<R, HTTPMethod::kDelete>;

template <concepts::RequestBody R>
using GET = detail::Request<R, HTTPMethod::kGet>;

template <concepts::RequestBody R>
using HEAD = detail::Request<R, HTTPMethod::kHead>;

template <concepts::RequestBody R>
using OPTIONS = detail::Request<R, HTTPMethod::kOptions>;

template <concepts::RequestBody R>
using PATCH = detail::Request<R, HTTPMethod::kPatch>;

template <concepts::RequestBody R>
using POST = detail::Request<R, HTTPMethod::kPost>;

template <concepts::RequestBody R>
using PUT = detail::Request<R, HTTPMethod::kPut>;

template <concepts::RequestBody R>
using TRACE = detail::Request<R, HTTPMethod::kTrace>;

template <concepts::RequestBody R>
using TRACK = detail::Request<R, HTTPMethod::kTrack>;

namespace detail {
template <typename Req, typename Resp, typename Err>
struct Entry {
  using Request = Req;
  using Response = Resp;
  using Error = Err;
};
}  // namespace detail

template <concepts::Request Request>
struct ForRequest {
  template <concepts::Response Response, concepts::Response... Responses>
  struct RespondsWith {
    template <concepts::Error Error, concepts::Error... Errors>
    using ErrorsWith =
        detail::Entry<Request,
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

template <concepts::detail::Entry... Entries>
struct Endpoint {
  static_assert(unique_types<typename Entries::Request...>::value,
                "Duplicate ForRequest<...>: each Request type may appear at "
                "most once in an Endpoint.");

  using Request = std::variant<typename Entries::Request...>;

  template <concepts::Request Request>
  static constexpr bool accepts_v =
      (std::is_same_v<Request, typename Entries::Request> || ...);

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

  template <concepts::Request Request>
  using EntryFor = typename Find<Request, Entries...>::type;

  template <concepts::Request Request>
  using ResponseFor = typename EntryFor<Request>::Response;

  template <concepts::Request Request>
  using ErrorFor = typename EntryFor<Request>::Error;

  template <concepts::Request Request>
  using ExpectedFor = base::expected<std::optional<ResponseFor<Request>>,
                                     std::optional<ErrorFor<Request>>>;

  template <concepts::Request Request>
  using CallbackFor = base::OnceCallback<void(int, ExpectedFor<Request>)>;
};

namespace concepts {
template <typename Request, typename Endpoint>
concept SupportedBy = Endpoint::template accepts_v<Request>;
}  // namespace concepts

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_BUILDER_H_

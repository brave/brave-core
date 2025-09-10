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

namespace endpoints::detail {

template <typename, template <typename...> typename>
struct is_specialization_of : std::false_type {};

template <typename... Ts, template <typename...> typename TT>
struct is_specialization_of<TT<Ts...>, TT> : std::true_type {};

template <typename T, template <typename...> typename TT>
inline constexpr bool is_specialization_of_v =
    is_specialization_of<T, TT>::value;

template <typename T, typename... Ts>
using Collapse =
    std::conditional_t<sizeof...(Ts) == 0, T, std::variant<T, Ts...>>;

template <typename... Ts>
struct inherit : Ts... {};

template <typename... Ts>
inline constexpr bool unique_types_v =
    requires { inherit<std::type_identity<Ts>...>{}; };

template <typename, typename, typename>
struct Entry;

}  // namespace endpoints::detail

namespace endpoints::detail::concepts {

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

template <typename T>
concept Request = ToValue<T> && Method<T> && Headers<T>;

template <typename T>
concept Response = FromValue<T>;

template <typename T>
concept Error = FromValue<T>;

template <typename T>
concept Endpoint = URL<T>;

template <typename T>
concept Entry = is_specialization_of_v<T, detail::Entry>;

}  // namespace endpoints::detail::concepts

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
template <typename Body, HTTPMethod M>
  requires concepts::ToValue<Body>
struct Request : Body {
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
      static_assert(base::AlwaysFalse<std::integral_constant<HTTPMethod, M>>,
                    "Unhandled HTTP method enum!");
    }
  }

  net::HttpRequestHeaders headers;
};
}  // namespace detail

template <typename Body>
  requires detail::concepts::ToValue<Body>
using CONNECT = detail::Request<Body, HTTPMethod::kConnect>;

template <typename Body>
  requires detail::concepts::ToValue<Body>
using DELETE = detail::Request<Body, HTTPMethod::kDelete>;

template <typename Body>
  requires detail::concepts::ToValue<Body>
using GET = detail::Request<Body, HTTPMethod::kGet>;

template <typename Body>
  requires detail::concepts::ToValue<Body>
using HEAD = detail::Request<Body, HTTPMethod::kHead>;

template <typename Body>
  requires detail::concepts::ToValue<Body>
using OPTIONS = detail::Request<Body, HTTPMethod::kOptions>;

template <typename Body>
  requires detail::concepts::ToValue<Body>
using PATCH = detail::Request<Body, HTTPMethod::kPatch>;

template <typename Body>
  requires detail::concepts::ToValue<Body>
using POST = detail::Request<Body, HTTPMethod::kPost>;

template <typename Body>
  requires detail::concepts::ToValue<Body>
using PUT = detail::Request<Body, HTTPMethod::kPut>;

template <typename Body>
  requires detail::concepts::ToValue<Body>
using TRACE = detail::Request<Body, HTTPMethod::kTrace>;

template <typename Body>
  requires detail::concepts::ToValue<Body>
using TRACK = detail::Request<Body, HTTPMethod::kTrack>;

namespace detail {
template <typename Req, typename Rsp, typename Err>
struct Entry {
  using Request = Req;
  using Response = Rsp;
  using Error = Err;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;
  using Callback = base::OnceCallback<void(int, Expected)>;
};

}  // namespace detail

template <detail::concepts::Request Req>
struct For {
  template <detail::concepts::Response Rsp, detail::concepts::Response... Rsps>
  struct RespondsWith {
    template <detail::concepts::Error Err, detail::concepts::Error... Errs>
    using ErrorsWith = detail::Entry<Req,
                                     detail::Collapse<Rsp, Rsps...>,
                                     detail::Collapse<Err, Errs...>>;
  };
};

template <detail::concepts::Entry... Entries>
  requires(detail::unique_types_v<typename Entries::Request...>)
class Endpoint {
  template <typename, typename...>
  struct EntryForImpl : std::type_identity<void> {};

  template <typename Req, typename E, typename... Es>
  struct EntryForImpl<Req, E, Es...>
      : std::conditional_t<std::is_same_v<Req, typename E::Request>,
                           std::type_identity<E>,
                           EntryForImpl<Req, Es...>> {};

 public:
  template <detail::concepts::Request Req>
  static constexpr bool IsSupported =
      (std::is_same_v<Req, typename Entries::Request> || ...);

  template <typename Req>
    requires IsSupported<Req>
  using EntryFor = typename EntryForImpl<Req, Entries...>::type;
};

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_BUILDER_H_

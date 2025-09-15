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
#include "base/types/is_instantiation.h"
#include "base/types/same_as_any.h"
#include "base/values.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"

namespace endpoints {
template <typename>
struct WithHeaders;
}  // namespace endpoints

namespace endpoints::detail {
template <typename T>
concept HasToValue = requires(T t) { base::WriteJson(t.ToValue()); } &&
                     std::is_member_function_pointer_v<decltype(&T::ToValue)>;

template <typename T>
concept HasFromValue = requires(const base::Value& v) {
  { T::FromValue(v) } -> std::same_as<std::optional<T>>;
};

template <typename T>
concept URL = requires {
  { T::URL() } -> std::same_as<GURL>;
};

template <typename T>
concept HasMethod = requires {
  { T::Method() } -> std::same_as<std::string_view>;
};

template <typename T>
concept HasRequestHeaders = requires(T& t) {
  { t.headers } -> std::same_as<net::HttpRequestHeaders&>;
};

template <typename T>
concept HasResponseHeaders = requires(T& t) {
  { t.headers } -> std::same_as<scoped_refptr<net::HttpResponseHeaders>&>;
};

template <typename T>
struct RequestImpl {
  static constexpr bool value = HasToValue<T> && HasMethod<T>;
};

template <typename T>
struct RequestImpl<WithHeaders<T>> {
  static constexpr bool value = RequestImpl<T>::value;
};

template <typename T>
concept Request = RequestImpl<T>::value;

template <typename T>
struct ResponseImpl {
  static constexpr bool value = HasFromValue<T>;
};

template <typename T>
struct ResponseImpl<WithHeaders<T>> {
  static constexpr bool value = ResponseImpl<T>::value;
};

template <typename T>
concept Response = ResponseImpl<T>::value;

template <typename T>
concept Error = Response<T>;

template <typename T>
concept Endpoint = URL<T>;

template <typename... Ts>
concept UniqueTypes = requires {
  [] {
    struct Unique : std::type_identity<Ts>... {};
  };
};

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

template <typename Body, HTTPMethod M>
struct WithMethod : Body {
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
                    "Unhandled HTTPMethod enum!");
    }
  }
};

template <typename Req, typename Rsp, typename Err>
struct Entry {
  using Request = Req;
  using Response = Rsp;
  using Error = Err;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;
  using Callback = base::OnceCallback<void(Expected)>;
};

}  // namespace endpoints::detail

namespace endpoints {
template <detail::Request Req>
struct WithHeaders<Req> : Req {
  net::HttpRequestHeaders headers;
};

template <detail::Response Rsp>
struct WithHeaders<Rsp> : Rsp {
  scoped_refptr<net::HttpResponseHeaders> headers;
};
}  // namespace endpoints

namespace endpoints {

template <detail::HasToValue Body>
using CONNECT = detail::WithMethod<Body, detail::HTTPMethod::kConnect>;

template <detail::HasToValue Body>
using DELETE = detail::WithMethod<Body, detail::HTTPMethod::kDelete>;

template <detail::HasToValue Body>
using GET = detail::WithMethod<Body, detail::HTTPMethod::kGet>;

template <detail::HasToValue Body>
using HEAD = detail::WithMethod<Body, detail::HTTPMethod::kHead>;

template <detail::HasToValue Body>
using OPTIONS = detail::WithMethod<Body, detail::HTTPMethod::kOptions>;

template <detail::HasToValue Body>
using PATCH = detail::WithMethod<Body, detail::HTTPMethod::kPatch>;

template <detail::HasToValue Body>
using POST = detail::WithMethod<Body, detail::HTTPMethod::kPost>;

template <detail::HasToValue Body>
using PUT = detail::WithMethod<Body, detail::HTTPMethod::kPut>;

template <detail::HasToValue Body>
using TRACE = detail::WithMethod<Body, detail::HTTPMethod::kTrace>;

template <detail::HasToValue Body>
using TRACK = detail::WithMethod<Body, detail::HTTPMethod::kTrack>;

template <detail::Request Req>
struct For {
  template <detail::Response Rsp, detail::Response... Rsps>
  struct RespondsWith {
    template <detail::Error Err, detail::Error... Errs>
    using ErrorsWith = detail::Entry<
        Req,
        std::conditional_t<sizeof...(Rsps), std::variant<Rsp, Rsps...>, Rsp>,
        std::conditional_t<sizeof...(Errs), std::variant<Err, Errs...>, Err>>;
  };
};

template <base::is_instantiation<detail::Entry>... Entries>
  requires(detail::UniqueTypes<typename Entries::Request...>)
class Endpoint {
  template <typename, typename...>
  struct EntryForImpl : std::type_identity<void> {};

  template <typename Req, typename E, typename... Es>
  struct EntryForImpl<Req, E, Es...>
      : std::conditional_t<std::same_as<Req, typename E::Request>,
                           std::type_identity<E>,
                           EntryForImpl<Req, Es...>> {};

  template <detail::Request Req>
  static constexpr bool kIsSupported =
      base::SameAsAny<Req, typename Entries::Request...>;

 public:
  template <detail::Request Req>
    requires kIsSupported<Req>
  using EntryFor = typename EntryForImpl<Req, Entries...>::type;
};

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_BUILDER_H_

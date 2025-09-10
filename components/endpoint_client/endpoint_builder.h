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

namespace endpoints::detail {
template <typename T>
concept ToValue = requires(T t) { base::WriteJson(t.ToValue()); } &&
                  std::is_member_function_pointer_v<decltype(&T::ToValue)>;

template <typename T>
concept FromValue = requires(const base::Value& value) {
  { T::FromValue(value) } -> std::same_as<std::optional<T>>;
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

template <typename... Ts>
concept UniqueTypes = requires {
  [] {
    struct Unique : std::type_identity<Ts>... {};
  }();
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
struct MakeRequest : Body {
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

  net::HttpRequestHeaders headers;
};

template <typename Req, typename Rsp, typename Err>
struct Entry {
  using Request = Req;
  using Response = Rsp;
  using Error = Err;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;
  using Callback = base::OnceCallback<void(int, Expected)>;
};

}  // namespace endpoints::detail

namespace endpoints {

template <detail::ToValue Body>
using CONNECT = detail::MakeRequest<Body, detail::HTTPMethod::kConnect>;

template <detail::ToValue Body>
using DELETE = detail::MakeRequest<Body, detail::HTTPMethod::kDelete>;

template <detail::ToValue Body>
using GET = detail::MakeRequest<Body, detail::HTTPMethod::kGet>;

template <detail::ToValue Body>
using HEAD = detail::MakeRequest<Body, detail::HTTPMethod::kHead>;

template <detail::ToValue Body>
using OPTIONS = detail::MakeRequest<Body, detail::HTTPMethod::kOptions>;

template <detail::ToValue Body>
using PATCH = detail::MakeRequest<Body, detail::HTTPMethod::kPatch>;

template <detail::ToValue Body>
using POST = detail::MakeRequest<Body, detail::HTTPMethod::kPost>;

template <detail::ToValue Body>
using PUT = detail::MakeRequest<Body, detail::HTTPMethod::kPut>;

template <detail::ToValue Body>
using TRACE = detail::MakeRequest<Body, detail::HTTPMethod::kTrace>;

template <detail::ToValue Body>
using TRACK = detail::MakeRequest<Body, detail::HTTPMethod::kTrack>;

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

 public:
  template <detail::Request Req>
  static constexpr bool IsSupported =
      base::SameAsAny<Req, typename Entries::Request...>;

  template <detail::Request Req>
    requires IsSupported<Req>
  using EntryFor = typename EntryForImpl<Req, Entries...>::type;
};

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_BUILDER_H_

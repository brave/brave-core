/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_H_

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
#include "brave/brave_domains/service_domains.h"
#include "brave/components/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/endpoint_client/maybe_variant.h"
#include "brave/components/endpoint_client/request.h"
#include "brave/components/endpoint_client/response.h"

namespace endpoint_client::detail {

template <typename... Ts>
concept UniqueTypes = requires {
  [] {
    struct Unique : std::type_identity<Ts>... {};
  };
};

}  // namespace endpoint_client::detail

namespace endpoint_client {
namespace detail {

template <IsRequest Req, IsResponse Ok, IsResponse Err>
struct Entry {
  using Request = Req;
  using Response = Ok;
  using Error = Err;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;
};

}  // namespace detail

template <detail::IsRequest Request>
struct For {
  template <detail::IsResponse... Oks>
    requires(sizeof...(Oks) > 0)
  struct ReturnsWith {
    template <detail::IsResponse... Errs>
      requires(sizeof...(Errs) > 0)
    using FailsWith = detail::
        Entry<Request, detail::MaybeVariant<Oks...>, detail::MaybeVariant<Errs...>>;
  };
};

template <std::size_t N>
struct FixedString {
  constexpr FixedString(const char (&str)[N]) { std::copy_n(str, N, value); }
  char value[N];
};

template <FixedString Prefix,
          FixedString Path,
          base::is_instantiation<detail::Entry>... Entries>
  requires(detail::UniqueTypes<typename Entries::Request...>)
class Endpoint {
  template <typename, typename...>
  struct EntryForImpl {};

  template <detail::IsRequest Request, typename E, typename... Es>
  struct EntryForImpl<Request, E, Es...>
      : std::conditional_t<std::is_same_v<Request, typename E::Request>,
                           std::type_identity<E>,
                           EntryForImpl<Request, Es...>> {};

  template <detail::IsRequest Request>
  static constexpr bool kHasEntryFor =
      requires { typename EntryForImpl<Request, Entries...>::type; };

 public:
  template <typename T>
    requires kHasEntryFor<detail::MaybeStripWithHeaders<T>>
  using EntryFor =
      typename EntryForImpl<detail::MaybeStripWithHeaders<T>, Entries...>::type;

  template <typename T>
  static constexpr bool kIsRequestSupported =
      kHasEntryFor<detail::MaybeStripWithHeaders<T>>;

  template <typename Rsp, typename Req>
    requires kIsRequestSupported<Req>
  static constexpr bool kIsResponseSupportedForRequest =
      std::is_same_v<detail::MaybeStripWithHeaders<Rsp>,
                     typename EntryFor<Req>::Response>;

  template <typename Err, typename Req>
    requires kIsRequestSupported<Req>
  static constexpr bool kIsErrorSupportedForRequest =
      std::is_same_v<detail::MaybeStripWithHeaders<Err>,
                     typename EntryFor<Req>::Error>;

  static GURL URL() {
    return brave_domains::GetServicesDomainUrl(Prefix.value)
        .Resolve(Path.value);
  }
};

namespace detail {

template <typename>
inline constexpr bool kIsEndpoint = false;

template <FixedString Prefix,
          FixedString Path,
          base::is_instantiation<Entry>... Entries>
inline constexpr bool kIsEndpoint<Endpoint<Prefix, Path, Entries...>> = true;

template <typename T>
concept IsEndpoint = kIsEndpoint<T>;

}  // namespace detail

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_H_

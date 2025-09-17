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
#include "brave/components/endpoint_client/request.h"
#include "brave/components/endpoint_client/response.h"

namespace endpoints::detail {

template <typename Req, typename Rsp, typename Err>
struct Entry {
  using Request = Req;
  using Response = Rsp;
  using Error = Err;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;
  using Callback = base::OnceCallback<void(Expected)>;
};

template <typename... Ts>
concept UniqueTypes = requires {
  [] {
    struct Unique : std::type_identity<Ts>... {};
  };
};

}  // namespace endpoints::detail

namespace endpoints {

template <detail::Request Request>
struct For {
  template <detail::Response Ok, detail::Response... Oks>
  struct ReturnsWith {
    template <detail::Response Error, detail::Response... Errors>
    using FailsWith = detail::Entry<
        Request,
        std::conditional_t<sizeof...(Oks), std::variant<Ok, Oks...>, Ok>,
        std::conditional_t<sizeof...(Errors),
                           std::variant<Error, Errors...>,
                           Error>>;
  };
};

template <base::is_instantiation<detail::Entry>... Entries>
  requires(detail::UniqueTypes<typename Entries::Request...>)
class Endpoint {
  template <typename, typename...>
  struct EntryForImpl {};

  template <detail::Request Request, typename E, typename... Es>
  struct EntryForImpl<Request, E, Es...>
      : std::conditional_t<std::is_same_v<Request, typename E::Request>,
                           std::type_identity<E>,
                           EntryForImpl<Request, Es...>> {};

  template <detail::Request Request>
  static constexpr bool kHasEntryFor =
      requires { typename EntryForImpl<Request, Entries...>::type; };

 public:
  template <detail::Request Request>
    requires kHasEntryFor<Request>
  using EntryFor = typename EntryForImpl<Request, Entries...>::type;
};

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_BUILDER_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_RESULT_FOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_RESULT_FOR_H_

#include <type_traits>

#include "base/types/expected.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_endpoints.mojom.h"

namespace ledger::endpoints {

template <typename>
inline constexpr bool dependent_false_v = false;

template <typename Endpoint>
struct ResultFor {
  static_assert(dependent_false_v<Endpoint>,
                "Please use the RESULT_FOR() macro to explicitly "
                "specialize ResultFor<> for "
                "your endpoint!");
};

template <typename, typename = void>
inline constexpr bool error_type_check = false;

template <typename T>
inline constexpr bool
    error_type_check<T, std::void_t<decltype(T::kFailedToCreateRequest)>> =
        true;

}  // namespace ledger::endpoints

#define STRINGIFY(x) #x
// clang-format off
#define RESULT_FOR(endpoint, value_type)                                       \
  }                                                                            \
                                                                               \
  namespace mojom {                                                            \
  enum class endpoint##Error : int32_t;                                        \
                                                                               \
  static_assert(                                                               \
      endpoints::error_type_check<endpoint##Error>,                            \
      "Please define " STRINGIFY(endpoint##Error) " in ledger_endpoints.mojom" \
      " and make sure it has the enumerator kFailedToCreateRequest!");         \
  }                                                                            \
  /* NOLINTNEXTLINE */                                                         \
  namespace endpoints {                                                        \
  class endpoint;                                                              \
                                                                               \
  template <>                                                                  \
  struct ResultFor<endpoint> {                                                 \
    using Result = base::expected<value_type, mojom::endpoint##Error>;         \
  }
// clang-format on

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_RESULT_FOR_H_

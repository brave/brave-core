/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_BASE_H_

#include <type_traits>

#include "base/check.h"
#include "base/no_destructor.h"
#include "base/types/always_false.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/features.h"

namespace detail {
// A helper metafunction to deduce the context type
// used by a ProfileKeyedServiceFactory-like class.
//
// Given a PKSF (ProfileKeyedServiceFactory),
// this metafunction inspects whether it defines either:
//   - GetBrowserContextToUse(), or
//   - GetBrowserStateToUse()
// and infers the return type of that method, which is assumed
// to represent the "context type" for that factory.
//
// The inspection is done using a derived `Probe` class to gain
// access to protected methods, and uses `std::declval<>()` to
// simulate calling the relevant method without requiring an instance.
//
// If neither method is found, compilation fails with a static_assert.
//
// Usage:
//   using Context = GetContext<ProfileKeyedServiceFactoryIOS>;
//   // returns web::BrowserState*
//
// This utility imposes no dependency on the actual factory base
// classes themselves - it works purely through interface inspection.
template <typename PKSF>
struct GetContextImpl {
  struct Probe : PKSF {
    friend struct GetContextImpl;
  };

  using type = decltype([] {
    if constexpr (requires(Probe p) { p.GetBrowserContextToUse(nullptr); }) {
      return std::type_identity<
          decltype(std::declval<Probe>().GetBrowserContextToUse(nullptr))>{};
    } else if constexpr (requires(Probe p) {
                           p.GetBrowserStateToUse(nullptr);
                         }) {
      return std::type_identity<
          decltype(std::declval<Probe>().GetBrowserStateToUse(nullptr))>{};
    } else {
      static_assert(base::AlwaysFalse<PKSF>,
                    "PKSF must define GetBrowserContextToUse() or "
                    "GetBrowserStateToUse()!");
      return std::type_identity<void>{};
    }
  }())::type;
};

template <typename PKSF>
using GetContext = typename GetContextImpl<PKSF>::type;

}  // namespace detail

namespace brave_account {

template <typename BASF, typename PKSF>
class BraveAccountServiceFactoryBase : public PKSF {
 public:
  BraveAccountServiceFactoryBase(const BraveAccountServiceFactoryBase&) =
      delete;
  BraveAccountServiceFactoryBase& operator=(
      const BraveAccountServiceFactoryBase&) = delete;

  static BASF* GetInstance() {
    static base::NoDestructor<BASF> instance;
    return instance.get();
  }

  static BraveAccountService* GetFor(detail::GetContext<PKSF> context) {
    CHECK(context);
    return static_cast<BraveAccountService*>(
        GetInstance()->GetServiceForContext(context, true));
  }

 protected:
  BraveAccountServiceFactoryBase() : PKSF("BraveAccountService") {
    CHECK(brave_account::features::IsBraveAccountEnabled());
  }

  ~BraveAccountServiceFactoryBase() override = default;

 private:
  friend base::NoDestructor<BASF>;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_BASE_H_

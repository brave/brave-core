/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PROFILE_KEYED_SERVICE_FACTORY_SHIM_PROFILE_KEYED_SERVICE_FACTORY_SHIM_H_
#define BRAVE_COMPONENTS_PROFILE_KEYED_SERVICE_FACTORY_SHIM_PROFILE_KEYED_SERVICE_FACTORY_SHIM_H_

#include <memory>
#include <type_traits>
#include <utility>

#include "base/types/always_false.h"
#include "components/keyed_service/core/keyed_service.h"

template <typename PKSF, typename Context, typename Shim>
struct OverrideBuildServiceInstanceForBrowserContext : PKSF {
  using PKSF::PKSF;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      Context context) const override {
    return static_cast<const Shim*>(this)->BuildServiceInstanceForContext(
        context);
  }
};

template <typename PKSF, typename Context, typename Shim>
struct OverrideGetBrowserStateToUse : PKSF {
  using PKSF::PKSF;

  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      Context context) const override {
    return static_cast<const Shim*>(this)->BuildServiceInstanceForContext(
        context);
  }
};

template <typename PKSF>
class ProfileKeyedServiceFactoryTraits {
  template <typename T>
  struct DeriveFrom : T {
    friend class ProfileKeyedServiceFactoryTraits;
  };

  template <typename ContextT>
  struct WithContext {
    using Context = ContextT;

    template <typename Shim>
    using Override =
        OverrideBuildServiceInstanceForBrowserContext<PKSF, Context, Shim>;
  };

  template <typename ContextT>
  struct WithState {
    using Context = ContextT;

    template <typename Shim>
    using Override = OverrideGetBrowserStateToUse<PKSF, Context, Shim>;
  };

  using Computed = decltype([] {
    if constexpr (requires(DeriveFrom<PKSF> d) {
                    d.GetBrowserContextToUse(nullptr);
                  }) {
      using Context =
          decltype(std::declval<DeriveFrom<PKSF>>().GetBrowserContextToUse(
              nullptr));
      return std::type_identity<WithContext<Context>>{};
    } else if constexpr (requires(DeriveFrom<PKSF> d) {
                           d.GetBrowserStateToUse(nullptr);
                         }) {
      using Context =
          decltype(std::declval<DeriveFrom<PKSF>>().GetBrowserStateToUse(
              nullptr));
      return std::type_identity<WithState<Context>>{};
    } else {
      static_assert(
          base::AlwaysFalse<PKSF>,
          "PKSF must define GetBrowserContextToUse or GetBrowserStateToUse!");
      return std::type_identity<void>{};
    }
  }())::type;

 public:
  using Context = typename Computed::Context;

  template <typename Shim>
  using Override = typename Computed::template Override<Shim>;
};

template <typename PKSF, typename Traits = ProfileKeyedServiceFactoryTraits<PKSF>>
struct ProfileKeyedServiceFactoryShim
    : Traits::template Override<ProfileKeyedServiceFactoryShim<PKSF>> {
  using Base = typename Traits::template Override<ProfileKeyedServiceFactoryShim<PKSF>>;
  using Base::Base;
  using Context = typename Traits::Context;

  virtual std::unique_ptr<KeyedService> BuildServiceInstanceForContext(
      Context context) const = 0;

  ~ProfileKeyedServiceFactoryShim() override = default;
};

#endif  // BRAVE_COMPONENTS_PROFILE_KEYED_SERVICE_FACTORY_SHIM_PROFILE_KEYED_SERVICE_FACTORY_SHIM_H_

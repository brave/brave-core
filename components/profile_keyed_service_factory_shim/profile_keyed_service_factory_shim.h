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

template <typename Base,
          typename Context,
          typename Shim,
          template <typename...> class... Overrides>
struct ComposeOverrides;

template <typename Base, typename Context, typename Shim>
struct ComposeOverrides<Base, Context, Shim> {
  using type = Base;
};

template <typename Base,
          typename Context,
          typename Shim,
          template <typename...> class First,
          template <typename...> class... Rest>
struct ComposeOverrides<Base, Context, Shim, First, Rest...> {
  using type =
      First<typename ComposeOverrides<Base, Context, Shim, Rest...>::type,
            Context,
            Shim>;
};

template <typename Base,
          typename Context,
          typename Shim,
          template <typename...> class... Overrides>
using ComposeOverridesT =
    typename ComposeOverrides<Base, Context, Shim, Overrides...>::type;

template <typename T>
concept HasBuildServiceInstanceForContext = requires(T t) {
  t.BuildServiceInstanceForContext(nullptr);
};

template <typename Base, typename Context, typename Shim>
struct OverrideBuildServiceInstanceForBrowserContext : Base {
  using Base::Base;
};

template <typename Base,
          typename Context,
          typename Shim>
requires HasBuildServiceInstanceForContext<Shim>
struct OverrideBuildServiceInstanceForBrowserContext<Base, Context, Shim>
    : Base {
  using Base::Base;

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
    using Overrides =
        ComposeOverridesT<PKSF,
                          Context,
                          Shim,
                          OverrideBuildServiceInstanceForBrowserContext>;
  };

  template <typename ContextT>
  struct WithState {
    using Context = ContextT;

    template <typename Shim>
    using Overrides = OverrideGetBrowserStateToUse<PKSF, Context, Shim>;
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
  using Overrides = typename Computed::template Overrides<Shim>;
};

template <typename Concrete,
          typename PKSF,
          typename Traits = ProfileKeyedServiceFactoryTraits<PKSF>,
          typename Base = typename Traits::template Overrides<Concrete>>
struct ProfileKeyedServiceFactoryShim : Base {
  using Base::Base;
  using Context = typename Traits::Context;

  ~ProfileKeyedServiceFactoryShim() override = default;
};

#endif  // BRAVE_COMPONENTS_PROFILE_KEYED_SERVICE_FACTORY_SHIM_PROFILE_KEYED_SERVICE_FACTORY_SHIM_H_

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_COMMON_SCOPED_HMENU_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_COMMON_SCOPED_HMENU_H_

#include <windows.h>

#include "base/scoped_generic.h"

namespace brave {

namespace win {

namespace internal {

template <class T>
struct ScopedHMENUObjectTraits {
  static T InvalidValue() { return nullptr; }
  static void Free(T object) { DestroyMenu(object); }
};

}  // namespace internal

template <class T>
using ScopedHMENUObject =
    base::ScopedGeneric<T, internal::ScopedHMENUObjectTraits<T>>;

typedef brave::win::ScopedHMENUObject<HMENU> ScopedHMENU;

}  // namespace win
}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_COMMON_SCOPED_HMENU_H_

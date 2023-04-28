/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_COMMON_WIN_SCOPED_SC_HANDLE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_COMMON_WIN_SCOPED_SC_HANDLE_H_

#include "base/types/expected.h"
#include "base/win/scoped_handle.h"

// Helper for methods which perform system operations which may fail. The
// failure reason is returned as an HRESULT.
// TODO(crbug.com/1369769): Remove the following warning once resolved in
// base. NOTE: When ValueT is an integral type, base::expected's implicit ctors
// are ambiguous. To return an error in this case it must be wrapped in a
// base::unexpected(error);
template <typename ValueT>
using HResultOr = base::expected<ValueT, HRESULT>;

class ScHandleTraits {
 public:
  using Handle = SC_HANDLE;

  ScHandleTraits() = delete;
  ScHandleTraits(const ScHandleTraits&) = delete;
  ScHandleTraits& operator=(const ScHandleTraits&) = delete;

  static bool CloseHandle(SC_HANDLE handle) {
    return ::CloseServiceHandle(handle) != FALSE;
  }

  static bool IsHandleValid(SC_HANDLE handle) { return handle != nullptr; }

  static SC_HANDLE NullHandle() { return nullptr; }
};

using ScopedScHandle =
    base::win::GenericScopedHandle<ScHandleTraits,
                                   base::win::DummyVerifierTraits>;

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_COMMON_WIN_SCOPED_SC_HANDLE_H_

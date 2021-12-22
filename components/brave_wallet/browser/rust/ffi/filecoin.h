/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_RUST_FILECOIN_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_RUST_FILECOIN_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/rust/ffi/filecoin_ffi.h"

#if defined(FILECOIN_SHARED_LIBRARY)
#if defined(WIN32)
#define FILECOIN_EXPORT __declspec(dllexport)
#else  // defined(WIN32)
#define FILECOIN_EXPORT __attribute__((visibility("default")))
#endif
#else  // defined(FILECOIN_SHARED_LIBRARY)
#define FILECOIN_EXPORT
#endif

namespace filecoin {

class FILECOIN_EXPORT Filecoin {
 public:
  Filecoin();
  ~Filecoin();

  Filecoin(const Filecoin&) = delete;
  void operator=(const Filecoin&) = delete;

  /// Finish processing input and "close" the `Rewriter`. Flushes any input not
  /// yet processed and deallocates some of the internal resources.
  int End();

 private:
};

}  // namespace filecoin

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_RUST_FILECOIN_H_

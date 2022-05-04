/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_CALLBACK_ADAPTER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_CALLBACK_ADAPTER_H_

#include <utility>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

// Functor that generates |base::OnceCallback| objects that wrap arbitrary
// function objects. The generated callbacks are scoped to the adapter, and will
// not be called when the adapter is destroyed. |CallbackAdapter| is primarily
// intended to allow |std::function|-style code to easily consume APIs that
// use |base::OnceCallback|.
class CallbackAdapter {
 public:
  CallbackAdapter();
  ~CallbackAdapter();

  // Returns a |base::OnceCallback| that wraps the specified function.
  template <typename F>
  auto operator()(F fn) {
    return base::BindOnce(&CallbackWrapper<F>::template BindAdapter<F>,
                          weak_factory_.GetWeakPtr(), std::move(fn));
  }

  // Converts a boolean value to a |mojom::Result| for interoperability with
  // code that requires |mojom::Result| values.
  static mojom::Result ResultCode(bool success) {
    return success ? mojom::Result::LEDGER_OK : mojom::Result::LEDGER_ERROR;
  }

 private:
  template <typename F>
  struct CallbackWrapper;

  template <typename... Args>
  struct CallbackWrapper<void(Args...)> {
    template <typename F>
    static auto BindAdapter(base::WeakPtr<CallbackAdapter> weak_ptr,
                            F fn,
                            Args... args) {
      if (weak_ptr) {
        fn(std::move(args)...);
      }
    }
  };

  template <typename C, typename... Args>
  struct CallbackWrapper<void (C::*)(Args...)>
      : public CallbackWrapper<void(Args...)> {};

  template <typename C, typename... Args>
  struct CallbackWrapper<void (C::*)(Args...) const>
      : public CallbackWrapper<void(Args...)> {};

  template <typename... Args>
  struct CallbackWrapper<void (*)(Args...)>
      : public CallbackWrapper<void(Args...)> {};

  template <typename F>
  struct CallbackWrapper
      : public CallbackWrapper<decltype(&std::decay_t<F>::operator())> {};

  base::WeakPtrFactory<CallbackAdapter> weak_factory_{this};
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_CALLBACK_ADAPTER_H_

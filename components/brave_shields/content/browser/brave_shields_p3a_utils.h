// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_SHIELDS_P3A_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_SHIELDS_P3A_UTILS_H_

#include "base/check.h"

namespace brave_shields {
// This code implements a lazy evaluation pattern for P3A logging calls with
// several key features:
//
// * Incognito Mode Handling - The P3AContextCheck ensures analytics are only
//   recorded in non-incognito contexts by verifying IsOffTheRecord().
//
// * Lazy Evaluation - The P3A_LAZY_CALLS macro prevents unnecessary evaluation
//   of logging parameters when P3A is disabled for the current context.
//
// * Usage Enforcement - The P3AResult class, marked with [[nodiscard]], along
//   with its used_ flag, ensures logging calls are properly handled within
//   the context check.
//
// * The P3A() macro provides a clean interface that automatically handles
//   all context checking logic.
//
// * The P3A_FUNC macro marks the target function to be used for reporting P3A
//   metrics. The marked functions can't be called outside the context checking.

// Only use this if you can't acquire context. Make sure to use it in
// non-incognito mode.
struct NotIncognito {
  bool IsOffTheRecord() const { return false; }
};

struct P3AContextCheck {
  template <typename Context>
  static bool From(const Context context) {
    if constexpr (std::is_pointer_v<Context>) {
      if (!context || context->IsOffTheRecord()) {
        return false;
      }
    } else {
      if (context.IsOffTheRecord()) {
        return false;
      }
    }
    return true;
  }
};

class P3AResult {
 public:
  ~P3AResult() { CHECK(used_); }

 private:
  friend struct P3ACallStream;
  bool used_ = false;
};

struct P3ACallStream {
  struct Voidify {
    void operator&(P3ACallStream&) {}
  };

  P3ACallStream& operator<<(P3AResult&& r) {
    r.used_ = true;
    return *this;
  }
};

#define P3A_LAZY_CALLS(stream, is_enabled) \
  !(is_enabled) ? (void)0 : ::brave_shields::P3ACallStream::Voidify() & (stream)

#define P3A(context)                               \
  P3A_LAZY_CALLS(::brave_shields::P3ACallStream(), \
                 ::brave_shields::P3AContextCheck::From(context))

#define P3A_FUNC [[nodiscard]] ::brave_shields::P3AResult

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_SHIELDS_P3A_UTILS_H_

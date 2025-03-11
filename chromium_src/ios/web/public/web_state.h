// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEB_STATE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEB_STATE_H_

#include <string_view>

#define callbacks_                                                           \
  callbacks_;                                                                \
                                                                             \
 public:                                                                     \
  template <typename Interface>                                              \
  void AddUntrustedInterface(                                                \
      std::string_view host,                                                 \
      base::RepeatingCallback<void(mojo::PendingReceiver<Interface>)>        \
          callback) {                                                        \
    CHECK(!host.empty());                                                    \
    untrusted_callbacks_.emplace(std::string(host), Interface::Name_);       \
                                                                             \
    AddInterface(                                                            \
        Interface::Name_,                                                    \
        base::BindRepeating(&WrapCallback<Interface>, std::move(callback))); \
  }                                                                          \
                                                                             \
  bool IsAllowedForOrigin(const GURL& origin,                                \
                          std::string_view interface_name);                  \
                                                                             \
 private:                                                                    \
  std::map<std::string, std::string, std::less<>> untrusted_callbacks_

#include "src/ios/web/public/web_state.h"  // IWYU pragma: export

#undef callbacks_

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEB_STATE_H_

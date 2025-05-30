// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEB_STATE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEB_STATE_H_

#include <set>
#include <string_view>

#include "ios/components/webui/web_ui_url_constants.h"
#include "url/gurl.h"

#define callbacks_                                                           \
  callbacks_;                                                                \
                                                                             \
 public:                                                                     \
  template <typename Interface>                                              \
  void AddUntrustedInterface(                                                \
      const GURL& url,                                                       \
      base::RepeatingCallback<void(mojo::PendingReceiver<Interface>)>        \
          callback) {                                                        \
    DCHECK(!url.is_empty() && url.is_valid() &&                              \
           url.SchemeIs(kChromeUIUntrustedScheme));                          \
    untrusted_callbacks_[url.host()].insert(Interface::Name_);               \
                                                                             \
    AddInterface(                                                            \
        Interface::Name_,                                                    \
        base::BindRepeating(&WrapCallback<Interface>, std::move(callback))); \
  }                                                                          \
                                                                             \
  void RemoveUntrustedInterface(const GURL& origin,                          \
                                std::string_view interface_name);            \
                                                                             \
  bool IsAllowedForOrigin(const GURL& origin,                                \
                          std::string_view interface_name);                  \
                                                                             \
 private:                                                                    \
  std::map<std::string, std::set<std::string>, std::less<>> untrusted_callbacks_

#include "src/ios/web/public/web_state.h"  // IWYU pragma: export

#undef callbacks_

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEB_STATE_H_

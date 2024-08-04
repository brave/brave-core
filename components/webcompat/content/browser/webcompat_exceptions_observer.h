/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_CONTENT_BROWSER_WEBCOMPAT_EXCEPTIONS_OBSERVER_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_CONTENT_BROWSER_WEBCOMPAT_EXCEPTIONS_OBSERVER_H_

namespace webcompat {

class WebcompatExceptionsObserver {
 public:
  virtual void OnWebcompatRulesUpdated() = 0;
  virtual ~WebcompatExceptionsObserver() = default;
};

}  // namespace webcompat

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_CONTENT_BROWSER_WEBCOMPAT_EXCEPTIONS_OBSERVER_H_

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_THEME_EVENT_ROUTER_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_THEME_EVENT_ROUTER_H_

#include <memory>

class Profile;

namespace extensions {

class BraveThemeEventRouter {
 public:
  static std::unique_ptr<BraveThemeEventRouter> Create();

  virtual ~BraveThemeEventRouter() {}

  virtual void OnBraveThemeTypeChanged(Profile* profile) = 0;
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_THEME_EVENT_ROUTER_H_

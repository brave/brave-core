/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_COOKIE_SETTINGS_FACTORY_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_COOKIE_SETTINGS_FACTORY_H_

#include <string>

#include "chrome/browser/content_settings/cookie_settings_factory.h"

namespace content_settings {
class BraveCookieSettings;
}

class Profile;

class BraveCookieSettingsFactory
    : public CookieSettingsFactory {
 public:
  static scoped_refptr<content_settings::BraveCookieSettings> GetForProfile(
      Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<BraveCookieSettingsFactory>;

  BraveCookieSettingsFactory();
  ~BraveCookieSettingsFactory() override;

  DISALLOW_COPY_AND_ASSIGN(BraveCookieSettingsFactory);
};

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_COOKIE_SETTINGS_FACTORY_H_
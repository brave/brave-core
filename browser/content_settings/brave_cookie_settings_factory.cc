/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/content_settings/brave_cookie_settings_factory.h"

#include "brave/components/content_settings/core/browser/brave_cookie_settings.h"
#include "content/public/browser/browser_thread.h"

// static
scoped_refptr<content_settings::BraveCookieSettings>
BraveCookieSettingsFactory::GetForProfile(Profile* profile) {
  return static_cast<content_settings::BraveCookieSettings*>(
  	CookieSettingsFactory::GetForProfile(profile).get());
}
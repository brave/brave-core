/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_SERVICE_H_

// Take all includes from the original file since we are redefining Init, which
// is too common of a name.

#include <map>
#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/themes/theme_helper.h"
#include "chrome/common/buildflags.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/extension_id.h"
#include "ui/base/theme_provider.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"

#define THEMES_THEME_SERVICE_H_   \
 private:                         \
  friend class BraveThemeService; \
                                  \
 public:

#define Init virtual Init
#include "../../../../../chrome/browser/themes/theme_service.h"
#undef Init
#undef THEMES_THEME_SERVICE_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_SERVICE_H_

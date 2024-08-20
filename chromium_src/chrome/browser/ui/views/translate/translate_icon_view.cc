/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/vector_icons/vector_icons.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "components/vector_icons/vector_icons.h"

#define vector_icons
#define kTranslateIcon_ChromiumImpl kTranslateIcon
#define kTranslateIcon kBraveTranslateIcon

#include "src/chrome/browser/ui/views/translate/translate_icon_view.cc"

#undef vector_icons
#undef kTranslateIcon
#define kTranslateIcon kTranslateIcon_ChromiumImpl
#undef kTranslateIcon_ChromiumImpl

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_DELEGATE_H_

#define AddNewContents                                                        \
  AddNewContents_ChromiumImpl(                                                \
      WebContents* source, std::unique_ptr<WebContents> new_contents,         \
      const GURL& target_url, WindowOpenDisposition disposition,              \
      const blink::mojom::WindowFeatures& window_features, bool user_gesture, \
      bool* was_blocked);                                                     \
  virtual WebContents* AddNewContents

#include "src/content/public/browser/web_contents_delegate.h"  // IWYU pragma: export

#undef AddNewContents

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_DELEGATE_H_

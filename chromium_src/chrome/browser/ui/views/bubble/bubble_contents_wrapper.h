// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BUBBLE_BUBBLE_CONTENTS_WRAPPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BUBBLE_BUBBLE_CONTENTS_WRAPPER_H_

#include <vector>

#include "chrome/browser/ui/views/bubble/bubble_contents_wrapper.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

#define PrimaryPageChanged                                                 \
  SetWebContentsAddNewContentsDelegate(                                    \
      base::WeakPtr<content::WebContentsDelegate> browser_delegate);       \
  void AddNewContents(content::WebContents* source,                        \
                      std::unique_ptr<content::WebContents> new_contents,  \
                      const GURL& target_url,                              \
                      WindowOpenDisposition disposition,                   \
                      const blink::mojom::WindowFeatures& window_features, \
                      bool user_gesture, bool* was_blocked) override;      \
  const std::vector<int32_t>& popup_ids() const {                          \
    return popup_ids_;                                                     \
  }                                                                        \
  void ClearPopupIds();                                                    \
  void PrimaryPageChanged

#define webui_resizes_host_        \
  webui_resizes_host_;             \
  std::vector<int32_t> popup_ids_; \
  base::WeakPtr<content::WebContentsDelegate> browser_delegate_
#include "src/chrome/browser/ui/views/bubble/bubble_contents_wrapper.h"  // IWYU pragma: export
#undef webui_resizes_host_
#undef PrimaryPageChanged

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BUBBLE_BUBBLE_CONTENTS_WRAPPER_H_

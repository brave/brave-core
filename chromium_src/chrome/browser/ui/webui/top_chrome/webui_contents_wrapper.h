/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TOP_CHROME_WEBUI_CONTENTS_WRAPPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TOP_CHROME_WEBUI_CONTENTS_WRAPPER_H_

#include <vector>

#include "chrome/browser/ui/browser.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"

#define HideCustomContextMenu                                                 \
  HideCustomContextMenu() {}                                                  \
  virtual content::WebContents* AddNewContents_ChromiumImpl(                  \
      content::WebContents* source,                                           \
      std::unique_ptr<content::WebContents> new_contents,                     \
      const GURL& target_url, WindowOpenDisposition disposition,              \
      const blink::mojom::WindowFeatures& window_features, bool user_gesture, \
      bool* was_blocked);                                                     \
  virtual void HideCustomContextMenu_Unused

#define PrimaryPageChanged                                                    \
  SetWebContentsAddNewContentsDelegate(                                       \
      base::WeakPtr<content::WebContentsDelegate> browser_delegate);          \
  const std::vector<int32_t>& popup_ids() const {                             \
    return popup_ids_;                                                        \
  }                                                                           \
  void ClearPopupIds();                                                       \
  content::WebContents* AddNewContents_ChromiumImpl(                          \
      content::WebContents* source,                                           \
      std::unique_ptr<content::WebContents> new_contents,                     \
      const GURL& target_url, WindowOpenDisposition disposition,              \
      const blink::mojom::WindowFeatures& window_features, bool user_gesture, \
      bool* was_blocked) override;                                            \
  void PrimaryPageChanged

#define webui_resizes_host_        \
  webui_resizes_host_;             \
  std::vector<int32_t> popup_ids_; \
  base::WeakPtr<content::WebContentsDelegate> browser_delegate_
#include "src/chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"  // IWYU pragma: export
#undef webui_resizes_host_
#undef PrimaryPageChanged
#undef HideCustomContextMenu

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TOP_CHROME_WEBUI_CONTENTS_WRAPPER_H_

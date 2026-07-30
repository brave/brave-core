/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_WEB_CONTENTS_DELEGATE_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_WEB_CONTENTS_DELEGATE_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "chrome/browser/ui/browser_web_contents_delegate/browser_web_contents_delegate.h"

class BraveBrowserWebContentsDelegate : public BrowserWebContentsDelegate {
 public:
  BraveBrowserWebContentsDelegate(
      BrowserWindowInterface* browser,
      ExclusiveAccessManager& exclusive_access_manager,
      chrome::BrowserCommandController& command_controller,
      UnloadController& unload_controller,
      web_app::AppBrowserController* app_browser_controller,
      BrowserWindow& window,
      DesktopBrowserWindowCapabilities& capabilities);
  BraveBrowserWebContentsDelegate(const BraveBrowserWebContentsDelegate&) =
      delete;
  BraveBrowserWebContentsDelegate& operator=(
      const BraveBrowserWebContentsDelegate&) = delete;
  ~BraveBrowserWebContentsDelegate() override;

  content::WebContents* AddNewContents(
      content::WebContents* source,
      std::unique_ptr<content::WebContents> new_contents,
      const GURL& target_url,
      WindowOpenDisposition disposition,
      const blink::mojom::WindowFeatures& window_features,
      bool user_gesture,
      bool* was_blocked) override;
  void UpdateTargetURL(content::WebContents* source, const GURL& url) override;
  void BeforeUnloadFired(content::WebContents* source,
                         bool proceed,
                         bool* proceed_to_fire_unload) override;
  bool ShouldSuppressDialogs(content::WebContents* source) override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      scoped_refptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override;

 private:
  const raw_ref<BrowserWindowInterface> browser_;
  const raw_ref<UnloadController> unload_controller_;
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_WEB_CONTENTS_DELEGATE_H_

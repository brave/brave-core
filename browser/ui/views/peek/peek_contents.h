/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTENTS_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTENTS_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "content/public/browser/web_contents_delegate.h"

class BrowserWebContentsDelegate;
class BrowserWindowInterface;
class Profile;

// Owns the WebContents shown in Peek and acts as its delegate. Same-tab
// navigations stay in Peek; everything else is handed to the browser.
class PeekContents : public content::WebContentsDelegate {
 public:
  class Delegate {
   public:
    virtual void OnPeekCloseRequested() = 0;
    virtual void OnPeekNavigationStateChanged() = 0;

   protected:
    virtual ~Delegate() = default;
  };

  PeekContents(BrowserWindowInterface& browser,
               Delegate& delegate,
               std::unique_ptr<content::WebContents> contents);
  PeekContents(const PeekContents&) = delete;
  PeekContents& operator=(const PeekContents&) = delete;
  ~PeekContents() override;

  static std::unique_ptr<content::WebContents> CreateWebContents(
      Profile* profile);

  content::WebContents* web_contents() { return web_contents_.get(); }

  // Hands over the contents, e.g. to the tab strip. The caller becomes
  // responsible for setting a new delegate.
  std::unique_ptr<content::WebContents> ReleaseWebContents();

  // content::WebContentsDelegate:
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params,
      base::OnceCallback<void(content::NavigationHandle&)>
          navigation_handle_callback) override;
  content::WebContents* AddNewContents(
      content::WebContents* source,
      std::unique_ptr<content::WebContents> new_contents,
      const GURL& target_url,
      WindowOpenDisposition disposition,
      const blink::mojom::WindowFeatures& window_features,
      bool user_gesture,
      bool* was_blocked) override;
  void CloseContents(content::WebContents* source) override;
  void NavigationStateChanged(content::WebContents* source,
                              content::InvalidateTypes changed_flags) override;
  void VisibleSecurityStateChanged(content::WebContents* source) override;
  bool HandleKeyboardEvent(content::WebContents* source,
                           const input::NativeWebKeyboardEvent& event) override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      scoped_refptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override;
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      content::MediaResponseCallback callback) override;
  bool CheckMediaAccessPermission(content::RenderFrameHost* render_frame_host,
                                  const url::Origin& security_origin,
                                  blink::mojom::MediaStreamType type) override;

 private:
  std::unique_ptr<content::WebContents> web_contents_;
  const raw_ref<BrowserWindowInterface> browser_;
  // Handles what Peek doesn't, e.g. file choosers and media permissions.
  const raw_ref<BrowserWebContentsDelegate> browser_delegate_;
  const raw_ref<Delegate> delegate_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTENTS_H_

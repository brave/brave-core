/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTENTS_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTENTS_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "content/public/browser/web_contents_delegate.h"

class BrowserWindowInterface;
class GURL;
class Profile;

// Owns the WebContents shown in Peek and acts as its delegate. Same-tab
// navigations stay in Peek.
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

  // Creates contents for a link to `url` in `source`, in the same storage
  // partition, e.g. the same container. Returns nullptr if the partition
  // can't be matched.
  static std::unique_ptr<content::WebContents> CreateWebContents(
      Profile* profile,
      const GURL& url,
      content::WebContents& source);

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
  void CloseContents(content::WebContents* source) override;
  void NavigationStateChanged(content::WebContents* source,
                              content::InvalidateTypes changed_flags) override;
  void VisibleSecurityStateChanged(content::WebContents* source) override;
  content::KeyboardEventProcessingResult PreHandleKeyboardEvent(
      content::WebContents* source,
      const input::NativeWebKeyboardEvent& event) override;

 private:
  std::unique_ptr<content::WebContents> web_contents_;
  const raw_ref<BrowserWindowInterface> browser_;
  const raw_ref<Delegate> delegate_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTENTS_H_

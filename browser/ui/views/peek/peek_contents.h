/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTENTS_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTENTS_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "components/download/public/common/download_item.h"
#include "components/permissions/permission_request_manager.h"
#include "components/web_modal/web_contents_modal_dialog_manager_delegate.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/web_contents_delegate.h"

class BrowserWebContentsDelegate;
class BrowserWindowInterface;
class GURL;
class Profile;

// Owns the WebContents shown in Peek and acts as its delegate. Same-tab
// navigations stay in Peek. Anything that needs a tab, e.g. permission
// prompts, popups, fullscreen or tab-modal dialogs, moves the contents into a
// tab first so the browser UI for it is attached to that tab.
class PeekContents : public content::WebContentsDelegate,
                     public web_modal::WebContentsModalDialogManagerDelegate,
                     public permissions::PermissionRequestManager::Observer,
                     public content::DownloadManager::Observer {
 public:
  class Delegate {
   public:
    virtual void OnPeekCloseRequested() = 0;
    virtual void OnPeekNavigationStateChanged() = 0;

    // Moves the contents into a tab, synchronously. Destroys the
    // PeekContents.
    virtual void OnPeekMoveToTabRequired() = 0;

    // Like `OnPeekMoveToTabRequired()`, but asynchronously. If nothing was
    // ever shown in Peek, closes it instead.
    virtual void OnPeekMoveToTabOrCloseRequested() = 0;

    // Moves focus out of the page into the card's toolbar (or back around).
    // Returns false if there is nowhere to move it.
    virtual bool OnPeekTakeFocus(bool reverse) = 0;

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
  // partition, e.g. the same container.
  static std::unique_ptr<content::WebContents> CreateWebContents(
      Profile* profile,
      const GURL& url,
      content::WebContents& source);

  // Restores what Peek restricted on `contents` once it's in a tab.
  static void OnMovedToTab(content::WebContents& contents);

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
  bool TakeFocus(content::WebContents* source, bool reverse) override;
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
  void EnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame,
      const blink::mojom::FullscreenOptions& options) override;
  void RequestPointerLock(content::WebContents* web_contents,
                          bool user_gesture,
                          bool last_unlocked_by_target) override;
  void RequestKeyboardLock(content::WebContents* web_contents,
                           bool esc_key_locked) override;

  // web_modal::WebContentsModalDialogManagerDelegate:
  void SetWebContentsBlocked(content::WebContents* web_contents,
                             bool blocked) override;
  web_modal::WebContentsModalDialogHost* GetWebContentsModalDialogHost(
      content::WebContents* web_contents) override;

  // permissions::PermissionRequestManager::Observer:
  void OnPromptCreationFailedHiddenTab() override;

  // content::DownloadManager::Observer:
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void ManagerGoingDown(content::DownloadManager* manager) override;

 private:
  void StopObserving();

  std::unique_ptr<content::WebContents> web_contents_;
  const raw_ref<BrowserWindowInterface> browser_;
  // Handles what Peek doesn't, e.g. file choosers and keyboard shortcuts.
  const raw_ref<BrowserWebContentsDelegate> browser_delegate_;
  const raw_ref<Delegate> delegate_;

  base::ScopedObservation<permissions::PermissionRequestManager,
                          permissions::PermissionRequestManager::Observer>
      permission_request_manager_observation_{this};
  base::ScopedObservation<content::DownloadManager,
                          content::DownloadManager::Observer>
      download_manager_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTENTS_H_

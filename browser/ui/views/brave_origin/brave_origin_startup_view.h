/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ORIGIN_BRAVE_ORIGIN_STARTUP_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ORIGIN_BRAVE_ORIGIN_STARTUP_VIEW_H_

#include "brave/components/brave_origin/buildflags/buildflags.h"

static_assert(BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED));

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/widget/widget_delegate.h"

class GURL;
class PrefService;
class Profile;
class ScopedKeepAlive;
class ScopedProfileKeepAlive;

namespace content {
class WebContents;
}  // namespace content

namespace views {
class WebView;
}  // namespace views

// Standalone modal window that shows the Brave Origin startup dialog.
// Class uses Brave* prefix because "Brave Origin" is the product name.
// Appears before the profile picker on is_brave_origin_branded builds,
// gating browser access until the user buys or restores a purchase.
class BraveOriginStartupView : public views::WidgetDelegate,
                               public content::WebContentsDelegate,
                               public content::WebContentsObserver {
 public:
  // Returns true if the startup dialog should be shown (purchase not yet
  // validated or SKU credentials missing).
  static bool ShouldShowDialog(PrefService* local_state);

  // Shows the startup dialog. |on_complete| is called when the user has
  // been validated and the dialog closes, to continue the startup flow.
  // |open_external| is called to open URLs in the default browser.
  // |attempt_exit| is called to terminate the browser when the dialog is
  // closed without validation.
  using OpenExternalCallback = base::RepeatingCallback<void(const GURL&)>;
  using ProfileCallback = base::OnceCallback<void(Profile*)>;
  using CreateProfilesCallback = base::RepeatingCallback<void(ProfileCallback)>;

  // |on_complete| runs when the user validates; |open_external| opens URLs
  // externally; |attempt_exit| exits the browser; |create_system_profile| and
  // |create_default_profile| load the required profiles asynchronously.
  static void Show(base::OnceClosure on_complete,
                   OpenExternalCallback open_external,
                   base::RepeatingClosure attempt_exit,
                   CreateProfilesCallback create_system_profile,
                   CreateProfilesCallback create_default_profile);
  static void Hide();
  static bool IsShowing();

  BraveOriginStartupView(const BraveOriginStartupView&) = delete;
  BraveOriginStartupView& operator=(const BraveOriginStartupView&) = delete;

 private:
  BraveOriginStartupView(base::OnceClosure on_complete,
                         OpenExternalCallback open_external,
                         base::RepeatingClosure attempt_exit,
                         CreateProfilesCallback create_system_profile,
                         CreateProfilesCallback create_default_profile);
  ~BraveOriginStartupView() override;

  void Display();
  void OnSystemProfileCreated(Profile* profile);
  void OnDefaultProfileCreated(Profile* profile);
  void MaybeInit();
  void Init(Profile* system_profile);
  void SetupWebUICallbacks();

  // Called by the WebUI handler when the user wants to open the buy window.
  void OpenBuyWindow();

  // Called by the WebUI handler when purchase is validated.
  void CloseAndProceed();

  // views::WidgetDelegate:
  views::View* GetContentsView() override;
  void WidgetIsZombie(views::Widget* widget) override;
  void WindowClosing() override;

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
  bool HandleContextMenu(content::RenderFrameHost& render_frame_host,
                         const content::ContextMenuParams& params) override;
  bool HandleKeyboardEvent(content::WebContents* source,
                           const input::NativeWebKeyboardEvent& event) override;

  // content::WebContentsObserver:
  void DOMContentLoaded(content::RenderFrameHost* render_frame_host) override;

  std::unique_ptr<ScopedKeepAlive> keep_alive_;
  std::unique_ptr<ScopedProfileKeepAlive> profile_keep_alive_;
  std::unique_ptr<views::WebView> web_view_;

  int profiles_loaded_count_ = 0;

  bool validated_ = false;
  base::OnceClosure on_complete_;
  OpenExternalCallback open_external_;
  base::RepeatingClosure attempt_exit_;
  CreateProfilesCallback create_system_profile_;
  CreateProfilesCallback create_default_profile_;

  views::UnhandledKeyboardEventHandler unhandled_keyboard_event_handler_;

  raw_ptr<Profile> system_profile_ = nullptr;
  raw_ptr<Profile> default_profile_ = nullptr;

  base::WeakPtrFactory<BraveOriginStartupView> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ORIGIN_BRAVE_ORIGIN_STARTUP_VIEW_H_

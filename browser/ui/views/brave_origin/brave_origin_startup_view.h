/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ORIGIN_BRAVE_ORIGIN_STARTUP_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ORIGIN_BRAVE_ORIGIN_STARTUP_VIEW_H_

#include "brave/components/brave_origin/buildflags/buildflags.h"

static_assert(BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED));

#include <memory>
#include <optional>

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
  using ProfileCallback = base::OnceCallback<void(Profile*)>;

  // Delegate interface for external dependencies. Implemented by the browser
  // startup code and easily mockable in tests.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Opens |url| in the user's default external browser.
    virtual void OpenExternal(const GURL& url) = 0;

    // Exits the browser process.
    virtual void AttemptExit() = 0;

    // Asynchronously creates the system profile and passes it to |callback|.
    virtual void CreateSystemProfile(ProfileCallback callback) = 0;

    // Asynchronously creates the default user profile and passes it to
    // |callback|.
    virtual void CreateDefaultProfile(ProfileCallback callback) = 0;
  };

  // Returns true if the startup dialog should be shown (purchase not yet
  // validated or SKU credentials missing). Also returns false when running
  // under test infrastructure (--test-type flag).
  static bool ShouldShowDialog(PrefService* local_state);

  // Override ShouldShowDialog() result for testing. Pass std::nullopt to
  // remove the override and restore normal behavior.
  static void SetShouldShowDialogForTesting(std::optional<bool> override);

  // Shows the startup dialog. |on_complete| is called when the user has
  // been validated and the dialog closes, to continue the startup flow.
  // |delegate| provides external operations (opening URLs, exiting, creating
  // profiles).
  static void Show(base::OnceClosure on_complete,
                   std::unique_ptr<Delegate> delegate);
  static void Hide();
  static bool IsShowing();

  // Simulates a successful validation for testing. Calls CloseAndProceed() on
  // the current instance, triggering the on_complete callback.
  static void ValidateForTesting();  // IN-TEST

  BraveOriginStartupView(const BraveOriginStartupView&) = delete;
  BraveOriginStartupView& operator=(const BraveOriginStartupView&) = delete;

 private:
  BraveOriginStartupView(base::OnceClosure on_complete,
                         std::unique_ptr<Delegate> delegate);
  ~BraveOriginStartupView() override;

  void Display();
  void OnSystemProfileCreated(Profile* profile);
  void OnDefaultProfileCreated(Profile* profile);
  void MaybeInit();
  void Init(Profile* profile);
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
  std::unique_ptr<Delegate> delegate_;

  int profiles_loaded_count_ = 0;

  bool validated_ = false;
  base::OnceClosure on_complete_;

  views::UnhandledKeyboardEventHandler unhandled_keyboard_event_handler_;

  raw_ptr<Profile> system_profile_ = nullptr;
  raw_ptr<Profile> default_profile_ = nullptr;

  base::WeakPtrFactory<BraveOriginStartupView> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ORIGIN_BRAVE_ORIGIN_STARTUP_VIEW_H_

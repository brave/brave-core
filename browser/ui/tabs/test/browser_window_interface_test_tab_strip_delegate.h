/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_TEST_BROWSER_WINDOW_INTERFACE_TEST_TAB_STRIP_DELEGATE_H_
#define BRAVE_BROWSER_UI_TABS_TEST_BROWSER_WINDOW_INTERFACE_TEST_TAB_STRIP_DELEGATE_H_

#include <memory>

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "components/sessions/core/session_id.h"
#include "url/gurl.h"

class BrowserActions;
class ExclusiveAccessManager;
class Profile;

namespace content {
class WebContents;
}

namespace tabs {
class TabInterface;
}

namespace user_education {
class FeaturePromoController;
}

namespace views {
class WebView;
}

// Implements TestTabStripModelDelegate with a mocked BrowserWindowInterface
class BrowserWindowInterfaceTestTabStripModelDelegate
    : public TestTabStripModelDelegate {
 public:
  class TestBrowserWindowInterface : public BrowserWindowInterface {
   public:
    explicit TestBrowserWindowInterface(Profile* profile);
    ~TestBrowserWindowInterface() override;
    TestBrowserWindowInterface(const TestBrowserWindowInterface&) = delete;
    TestBrowserWindowInterface& operator=(const TestBrowserWindowInterface&) =
        delete;

    // BrowserWindowInterface:
    views::WebView* GetWebView() override;
    Profile* GetProfile() override;
    void OpenGURL(const GURL& gurl, WindowOpenDisposition disposition) override;
    content::WebContents* OpenURL(
        const content::OpenURLParams& params,
        base::OnceCallback<void(content::NavigationHandle&)>
            navigation_handle_callback) override;
    const SessionID& GetSessionID() override;
    bool IsTabStripVisible() override;
    views::View* TopContainer() override;
    tabs::TabInterface* GetActiveTabInterface() override;
    BrowserWindowFeatures& GetFeatures() override;
    web_modal::WebContentsModalDialogHost*
    GetWebContentsModalDialogHostForWindow() override;
    bool IsActive() override;
    base::CallbackListSubscription RegisterDidBecomeActive(
        DidBecomeActiveCallback callback) override;
    base::CallbackListSubscription RegisterDidBecomeInactive(
        DidBecomeInactiveCallback callback) override;
    ExclusiveAccessManager* GetExclusiveAccessManager() override;
    BrowserActions* GetActions() override;
    BrowserWindowInterface::Type GetType() const override;
    user_education::FeaturePromoController* GetFeaturePromoController()
        override;

   private:
    raw_ptr<Profile> profile_;
    SessionID session_id_;
    std::unique_ptr<BrowserWindowFeatures> features_;

    using DidBecomeActiveCallbackList =
        base::RepeatingCallbackList<void(BrowserWindowInterface*)>;
    DidBecomeActiveCallbackList did_become_active_callback_list_;

    using DidBecomeInactiveCallbackList =
        base::RepeatingCallbackList<void(BrowserWindowInterface*)>;
    DidBecomeInactiveCallbackList did_become_inactive_callback_list_;
  };

  explicit BrowserWindowInterfaceTestTabStripModelDelegate(Profile* profile);
  ~BrowserWindowInterfaceTestTabStripModelDelegate() override;
  BrowserWindowInterfaceTestTabStripModelDelegate(
      const BrowserWindowInterfaceTestTabStripModelDelegate&) = delete;
  BrowserWindowInterfaceTestTabStripModelDelegate& operator=(
      const BrowserWindowInterfaceTestTabStripModelDelegate&) = delete;

  // TestTabStripModelDelegate:
  BrowserWindowInterface* GetBrowserWindowInterface() override;

 private:
  std::unique_ptr<BrowserWindowInterface> interface_;
};

#endif  // BRAVE_BROWSER_UI_TABS_TEST_BROWSER_WINDOW_INTERFACE_TEST_TAB_STRIP_DELEGATE_H_

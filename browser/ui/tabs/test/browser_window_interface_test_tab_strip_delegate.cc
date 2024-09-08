/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/test/browser_window_interface_test_tab_strip_delegate.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "url/gurl.h"

BrowserWindowInterfaceTestTabStripModelDelegate::TestBrowserWindowInterface::
    TestBrowserWindowInterface(Profile* profile)
    : profile_(profile),
      session_id_(SessionID::InvalidValue()),
      features_(BrowserWindowFeatures::CreateBrowserWindowFeatures()) {}

BrowserWindowInterfaceTestTabStripModelDelegate::TestBrowserWindowInterface::
    ~TestBrowserWindowInterface() = default;

views::WebView* BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::GetWebView() {
  return nullptr;
}

Profile* BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::GetProfile() {
  return profile_;
}

void BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::OpenGURL(const GURL& gurl,
                                         WindowOpenDisposition disposition) {}

content::WebContents* BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::OpenURL(
        const content::OpenURLParams& params,
        base::OnceCallback<void(content::NavigationHandle&)>
            navigation_handle_callback) {
  return nullptr;
}

const SessionID& BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::GetSessionID() {
  return session_id_;
}

bool BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::IsTabStripVisible() {
  return false;
}

views::View* BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::TopContainer() {
  return nullptr;
}

tabs::TabInterface* BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::GetActiveTabInterface() {
  return nullptr;
}

BrowserWindowFeatures& BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::GetFeatures() {
  return *features_;
}

web_modal::WebContentsModalDialogHost*
BrowserWindowInterfaceTestTabStripModelDelegate::TestBrowserWindowInterface::
    GetWebContentsModalDialogHostForWindow() {
  return nullptr;
}

bool BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::IsActive() {
  return false;
}

base::CallbackListSubscription BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::RegisterDidBecomeActive(
        DidBecomeActiveCallback callback) {
  return did_become_active_callback_list_.Add(std::move(callback));
}

base::CallbackListSubscription BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::RegisterDidBecomeInactive(
        DidBecomeInactiveCallback callback) {
  return did_become_inactive_callback_list_.Add(std::move(callback));
}

ExclusiveAccessManager* BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::GetExclusiveAccessManager() {
  return nullptr;
}

BrowserActions* BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::GetActions() {
  return nullptr;
}

BrowserWindowInterface::Type BrowserWindowInterfaceTestTabStripModelDelegate::
    TestBrowserWindowInterface::GetType() const {
  return TYPE_NORMAL;
}

user_education::FeaturePromoController*
BrowserWindowInterfaceTestTabStripModelDelegate::TestBrowserWindowInterface::
    GetFeaturePromoController() {
  return nullptr;
}

BrowserWindowInterfaceTestTabStripModelDelegate::
    BrowserWindowInterfaceTestTabStripModelDelegate(Profile* profile)
    : interface_(std::make_unique<TestBrowserWindowInterface>(profile)) {}

BrowserWindowInterfaceTestTabStripModelDelegate::
    ~BrowserWindowInterfaceTestTabStripModelDelegate() = default;

BrowserWindowInterface*
BrowserWindowInterfaceTestTabStripModelDelegate::GetBrowserWindowInterface() {
  return interface_.get();
}

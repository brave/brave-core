/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/opentabs/brave_tabgenerator_api.h"

#include <WebKit/WebKit.h>
#include <memory>

#include "base/strings/sys_string_conversions.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/main/browser.h"
#include "ios/chrome/browser/sessions/ios_chrome_session_tab_helper.h"
#include "ios/chrome/browser/tabs/synced_window_delegate_browser_agent.h"
#include "ios/chrome/browser/web_state_list/web_state_list.h"
#include "ios/chrome/browser/web_state_list/web_state_opener.h"

#include "ios/web/public/session/crw_navigation_item_storage.h"
#include "ios/web/public/session/crw_session_storage.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/web_state/ui/crw_web_view_navigation_proxy.h"
#include "ios/web/web_state/web_state_impl.h"

#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#pragma mark - BackForwardList

// Back Forward List for use in Navigation Manager
@interface BackForwardList : NSObject
@property(nonatomic, readonly, copy) WKBackForwardListItem* currentItem;
@property(nonatomic, readonly, copy) NSArray<WKBackForwardListItem*>* backList;
@property(nonatomic, readonly, copy)
    NSArray<WKBackForwardListItem*>* forwardList;
@end

@implementation BackForwardList
@synthesize currentItem;
@synthesize backList;
@synthesize forwardList;

- (instancetype)init {
  self = [super init];
  return self;
}

- (WKBackForwardListItem*)itemAtIndex:(NSInteger)index {
  if (index == 0) {
    return currentItem;
  }

  if (index > 0 && forwardList.count) {
    return forwardList[index - 1];
  }

  if (backList.count) {
    return backList[backList.count + index];
  }

  return nullptr;
}

- (WKBackForwardListItem*)backItem {
  return backList.lastObject;
}

- (WKBackForwardListItem*)forwardItem {
  return forwardList.firstObject;
}
@end

#pragma mark - NavigationProxy

@interface NavigationProxy : NSObject <CRWWebViewNavigationProxy>
@end

@implementation NavigationProxy
- (instancetype)init {
  if ((self = [super init])) {
  }
  return self;
}

- (NSURL*)URL {
  return net::NSURLWithGURL(GURL(url::kAboutBlankURL));
}

- (WKBackForwardList*)backForwardList {
  return (WKBackForwardList*)[[BackForwardList alloc] init];
}
@end

#pragma mark - NavigationDelegate

class NavigationDelegate : public web::NavigationManagerDelegate {
 public:
  NavigationDelegate(web::WebState* web_state);
  ~NavigationDelegate() override;
  void ClearDialogs() override;
  void RecordPageStateInNavigationItem() override;
  void LoadCurrentItem(web::NavigationInitiationType type) override;
  void LoadIfNecessary() override;
  void Reload() override;
  void OnNavigationItemCommitted(web::NavigationItem* item) override;
  web::WebState* GetWebState() override;
  void SetWebStateUserAgent(web::UserAgentType user_agent_type) override;
  id<CRWWebViewNavigationProxy> GetWebViewNavigationProxy() const override;
  void GoToBackForwardListItem(WKBackForwardListItem* wk_item,
                               web::NavigationItem* item,
                               web::NavigationInitiationType type,
                               bool has_user_gesture) override;

  void RemoveWebView() override;
  web::NavigationItemImpl* GetPendingItem() override;

 private:
  web::WebState* web_state_;
};

NavigationDelegate::~NavigationDelegate() = default;

id<CRWWebViewNavigationProxy> NavigationDelegate::GetWebViewNavigationProxy()
    const {
  return [[NavigationProxy alloc] init];
}

web::WebState* NavigationDelegate::GetWebState() {
  return web_state_;
}

web::NavigationItemImpl* NavigationDelegate::GetPendingItem() {
  return nullptr;
}

NavigationDelegate::NavigationDelegate(web::WebState* web_state)
    : web_state_(web_state) {
  // Not needed on iOS
}

void NavigationDelegate::ClearDialogs() {
  // Not needed on iOS
}

void NavigationDelegate::RecordPageStateInNavigationItem() {
  // Not needed on iOS
}

void NavigationDelegate::LoadCurrentItem(web::NavigationInitiationType type) {
  // Not needed on iOS
}

void NavigationDelegate::LoadIfNecessary() {
  // Not needed on iOS
}

void NavigationDelegate::Reload() {
  // Not needed on iOS
}

void NavigationDelegate::OnNavigationItemCommitted(web::NavigationItem* item) {
  // Not needed on iOS
}

void NavigationDelegate::SetWebStateUserAgent(
    web::UserAgentType user_agent_type) {
  // Not needed on iOS
}

void NavigationDelegate::GoToBackForwardListItem(
    WKBackForwardListItem* wk_item,
    web::NavigationItem* item,
    web::NavigationInitiationType type,
    bool has_user_gesture) {
  // Not needed on iOS
}

void NavigationDelegate::RemoveWebView() {
  // Not needed on iOS
}

#pragma mark - BraveNativeTab

class BraveNativeTab {
 public:
  BraveNativeTab(Browser* browser);
  ~BraveNativeTab();

  void SetTitle(const std::u16string& title);
  void SetURL(const GURL& url);

 private:
  class Observer : public web::WebStateObserver {
   public:
    explicit Observer(BraveNativeTab* tab);
    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;
    ~Observer() override;

   private:
    // WebStateObserver:
    void WebStateDestroyed(web::WebState* web_state) override;
    BraveNativeTab* tab_;  // NOT OWNED
  };

  Browser* browser_;
  SessionID session_id_;
  std::unique_ptr<NavigationDelegate> navigation_delegate_;
  web::WebState* web_state_;
  std::unique_ptr<Observer> web_state_observer_;
};

BraveNativeTab::BraveNativeTab(Browser* browser)
    : browser_(browser),
      session_id_(SessionID::InvalidValue()),
      navigation_delegate_(nullptr),
      web_state_(nullptr),
      web_state_observer_(nullptr) {
  // First setup SessionID for the tab
  session_id_ =
      SyncedWindowDelegateBrowserAgent::FromBrowser(browser_)->GetSessionId();

  // Create session storage with an empty item storage
  NSMutableArray<CRWNavigationItemStorage*>* item_storages =
      [[NSMutableArray alloc] init];
  CRWNavigationItemStorage* item = [[CRWNavigationItemStorage alloc] init];
  [item_storages addObject:item];

  CRWSessionStorage* session_storage = [[CRWSessionStorage alloc] init];
  session_storage.stableIdentifier = [[NSUUID UUID] UUIDString];
  session_storage.itemStorages = [item_storages copy];
  session_storage.userAgentType = web::UserAgentType::MOBILE;

  // Create WebState with parameters
  web::WebState::CreateParams create_params(browser->GetBrowserState());
  create_params.last_active_time = base::Time::Now();
  auto web_state =
      web::WebState::CreateWithStorageSession(create_params, session_storage);
  web_state->ForceRealized();

  // Setup Observers of the WebState
  web_state_ = web_state.get();
  web_state_observer_ = std::make_unique<Observer>(this);

  // Setup Navigation Delegate for the WebState
  navigation_delegate_ = std::make_unique<NavigationDelegate>(web_state_);
  static_cast<web::NavigationManagerImpl*>(web_state_->GetNavigationManager())
      ->SetDelegate(navigation_delegate_.get());

  // Insert the WebState into the Browser && Activate it
  browser_->GetWebStateList()->InsertWebState(
      browser_->GetWebStateList()->count(), std::move(web_state),
      WebStateList::INSERT_ACTIVATE, WebStateOpener());

  // Finally Set the WebState WindowID
  IOSChromeSessionTabHelper::FromWebState(web_state_)->SetWindowID(session_id_);
}

BraveNativeTab::~BraveNativeTab() {
  // Cleanup the WebState Observer
  web_state_observer_.reset();

  // Cleanup the WebState
  if (web_state_) {
    int index = browser_->GetWebStateList()->GetIndexOfWebState(web_state_);
    if (index >= 0) {
      browser_->GetWebStateList()->CloseWebStateAt(
          index, WebStateList::ClosingFlags::CLOSE_USER_ACTION);
    }
  }

  // Cleanup everything else in reverse order of construction
  navigation_delegate_.reset();
  web_state_ = nullptr;
  session_id_ = SessionID::InvalidValue();
  browser_ = nullptr;
}

void BraveNativeTab::SetTitle(const std::u16string& title) {
  DCHECK(web_state_);

  if (web_state_) {
    web::NavigationManager* navigation_manager =
        web_state_->GetNavigationManager();
    DCHECK(navigation_manager);

    web::NavigationItem* item = navigation_manager->GetPendingItem() ?: navigation_manager->GetLastCommittedItem() ?: navigation_manager->GetVisibleItem();

    if (item) {
      item->SetTitle(title);
      static_cast<web::WebStateImpl*>(web_state_)->OnTitleChanged();
    } else {
      VLOG(1) << "Invalid NavigationItem while setting up the tab";
    }
  } else {
    VLOG(1) << "Invalid WebState while setting up the tab";
  }
}

void BraveNativeTab::SetURL(const GURL& url) {
  DCHECK(web_state_);

  if (web_state_) {
    web::NavigationManagerImpl* navigation_manager =
        static_cast<web::NavigationManagerImpl*>(
            web_state_->GetNavigationManager());
    DCHECK(navigation_manager);

    navigation_manager->AddPendingItem(
        url, web::Referrer(), ui::PAGE_TRANSITION_TYPED,
        web::NavigationInitiationType::BROWSER_INITIATED, false,
        web::HttpsUpgradeType::kNone);

    navigation_manager->CommitPendingItem();
    static_cast<web::WebStateImpl*>(web_state_)->OnPageLoaded(url, true);
  } else {
    VLOG(1) << "Invalid WebState while setting up the tab url!";
  }
}

BraveNativeTab::Observer::Observer(BraveNativeTab* tab) : tab_(tab) {
  DCHECK(tab_->web_state_);
  tab_->web_state_->AddObserver(this);
}

BraveNativeTab::Observer::~Observer() {
  if (tab_->web_state_) {
    tab_->web_state_->RemoveObserver(this);
  }
  tab_ = nullptr;
}

void BraveNativeTab::Observer::WebStateDestroyed(web::WebState* web_state) {
  if (web_state == tab_->web_state_) {
    tab_->web_state_ = nullptr;
  }
  web_state->RemoveObserver(this);
}

#pragma mark - BraveSyncTab

@interface BraveSyncTab () {
  std::unique_ptr<BraveNativeTab> native_tab_;
}
@end

@implementation BraveSyncTab

- (instancetype)initWithBrowser:(Browser*)browser {
  if ((self = [super init])) {
    native_tab_ = std::make_unique<BraveNativeTab>(browser);
  }
  return self;
}

- (void)dealloc {
  native_tab_.reset();
}

- (void)setTitle:(NSString*)title {
  DCHECK(native_tab_);
  native_tab_->SetTitle(base::SysNSStringToUTF16(title));
}

- (void)setURL:(NSURL*)url {
  DCHECK(native_tab_);
  native_tab_->SetURL(net::GURLWithNSURL(url));
}
@end

#pragma mark - BraveTabGeneratorAPI

@interface BraveTabGeneratorAPI () {
  Browser* browser_;
}
@end

@implementation BraveTabGeneratorAPI

- (instancetype)initWithBrowser:(Browser*)browser {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    DCHECK(browser);
    browser_ = browser;
  }
  return self;
}

- (BraveSyncTab*)createBraveSyncTab {
  return [[BraveSyncTab alloc] initWithBrowser:browser_];
}

- (void)dealloc {
  browser_ = nil;
}

@end

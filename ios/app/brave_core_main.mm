/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/app/brave_core_main.h"

#include "base/files/file_path.h"
#include "base/mac/bundle_locations.h"
#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/app/brave_main_delegate.h"
#import "brave/ios/browser/brave_web_client.h"
#import "ios/chrome/app/startup_tasks.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/app/startup/provider_registration.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#include "ios/web/public/init/web_main.h"

@interface BraveCoreMain()
{
  std::unique_ptr<BraveWebClient> _webClient;
  std::unique_ptr<BraveMainDelegate> _delegate;
  std::unique_ptr<web::WebMain> _webMain;
  ChromeBrowserState* _mainBrowserState;
}
@end

@implementation BraveCoreMain

- (instancetype)init {
  if ((self = [super init])) {
    // TODO(bridiver) - this should probably go in BraveMainDelegate
    base::FilePath path;
    base::PathService::Get(base::DIR_MODULE, &path);
    base::mac::SetOverrideFrameworkBundlePath(path);

    // Register all providers before calling any Chromium code.
    [ProviderRegistration registerProviders];

    _webClient.reset(new BraveWebClient());
    web::SetWebClient(_webClient.get());

    _delegate.reset(new BraveMainDelegate());

    web::WebMainParams params(_delegate.get());

    _webMain = std::make_unique<web::WebMain>(std::move(params));

    ios::GetChromeBrowserProvider()->Initialize();

    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    ChromeBrowserState* chromeBrowserState =
        browserStateManager->GetLastUsedBrowserState();
    _mainBrowserState = chromeBrowserState;
  }
  return self;
}

- (void)scheduleLowPriorityStartupTasks {
  [StartupTasks
      scheduleDeferredBrowserStateInitialization:_mainBrowserState];
}

- (void)dealloc {
  _mainBrowserState = nullptr;
  _webMain.reset();
  _delegate.reset();
  _webClient.reset();
}

- (void)setUserAgent:(NSString *)userAgent {
  _webClient->SetUserAgent(base::SysNSStringToUTF8(userAgent));
}

@end

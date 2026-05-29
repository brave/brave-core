/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"

#import <WebKit/WebKit.h>

#include <memory>

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/constants/webui_url_constants.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

@interface BraveWalletWebUILocalStorageCleaner : NSObject <WKNavigationDelegate>
+ (void)clearForProfile:(ProfileIOS*)profile;
@end

@implementation BraveWalletWebUILocalStorageCleaner {
  WKWebView* _webView;
}

static BraveWalletWebUILocalStorageCleaner* currentCleaner;

+ (void)clearForProfile:(ProfileIOS*)profile {
  dispatch_async(dispatch_get_main_queue(), ^{
    WKWebViewConfiguration* config =
        [web::WKWebViewConfigurationProvider::FromBrowserState(profile)
                .GetWebViewConfiguration() copy];
    WKWebView* webView = [[WKWebView alloc] initWithFrame:CGRectZero
                                            configuration:config];
    BraveWalletWebUILocalStorageCleaner* cleaner =
        [[BraveWalletWebUILocalStorageCleaner alloc] init];
    cleaner->_webView = webView;
    webView.navigationDelegate = cleaner;
    currentCleaner = cleaner;
    [webView loadRequest:[NSURLRequest
                             requestWithURL:
                                 [NSURL URLWithString:@(kBraveUIWalletURL)]]];
  });
}

- (void)done {
  _webView.navigationDelegate = nil;
  _webView = nil;
  currentCleaner = nil;
}

- (void)webView:(WKWebView*)webView
    didFinishNavigation:(WKNavigation*)navigation {
  [webView evaluateJavaScript:@"localStorage.clear()"
            completionHandler:^(id result, NSError* error) {
              [self done];
            }];
}

- (void)webView:(WKWebView*)webView
    didFailNavigation:(WKNavigation*)navigation
            withError:(NSError*)error {
  [self done];
}

- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
                       withError:(NSError*)error {
  [self done];
}

- (void)webViewWebContentProcessDidTerminate:(WKWebView*)webView {
  [self done];
}

@end

namespace brave_wallet {

class BraveWalletServiceDelegateIos : public BraveWalletServiceDelegate {
 public:
  explicit BraveWalletServiceDelegateIos(ProfileIOS* profile)
      : profile_(profile) {
    wallet_base_directory_ = profile->GetStatePath();
    is_private_window_ = profile->IsOffTheRecord();
  }

  void ClearWalletUIStoragePartition() override {
    [BraveWalletWebUILocalStorageCleaner clearForProfile:profile_];
  }

  base::FilePath GetWalletBaseDirectory() override {
    return wallet_base_directory_;
  }
  bool IsPrivateWindow() override { return is_private_window_; }
  bool IsAutolockEnabled() override { return true; }

 protected:
  raw_ptr<ProfileIOS> profile_;
  base::FilePath wallet_base_directory_;
  bool is_private_window_ = false;
};

// static
BraveWalletService* BraveWalletServiceFactory::GetServiceForState(
    ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<BraveWalletService>(profile,
                                                                   true);
}

// static
BraveWalletServiceFactory* BraveWalletServiceFactory::GetInstance() {
  static base::NoDestructor<BraveWalletServiceFactory> instance;
  return instance.get();
}

BraveWalletServiceFactory::BraveWalletServiceFactory()
    : ProfileKeyedServiceFactoryIOS("BraveWalletService",
                                    ProfileSelection::kNoInstanceInIncognito,
                                    ServiceCreation::kCreateLazily,
                                    TestingCreation::kNoServiceForTests) {}

BraveWalletServiceFactory::~BraveWalletServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveWalletServiceFactory::BuildServiceInstanceFor(ProfileIOS* profile) const {
  std::unique_ptr<BraveWalletService> service(new BraveWalletService(
      profile->GetSharedURLLoaderFactory(),
      std::make_unique<BraveWalletServiceDelegateIos>(profile),
      profile->GetPrefs(), GetApplicationContext()->GetLocalState()));
  return service;
}

}  // namespace brave_wallet

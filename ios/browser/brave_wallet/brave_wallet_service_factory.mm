/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"

#import <WebKit/WebKit.h>

#include <memory>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/constants/webui_url_constants.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

@interface WalletWebViewRunnerDelegate : NSObject <WKNavigationDelegate> {
  base::OnceClosure _onDone;
}
- (instancetype)initWithClosure:(base::OnceClosure)closure;
@end

@implementation WalletWebViewRunnerDelegate

- (instancetype)initWithClosure:(base::OnceClosure)closure {
  self = [super init];
  if (self) {
    _onDone = std::move(closure);
  }
  return self;
}

- (void)done {
  if (_onDone) {
    std::move(_onDone).Run();
  }
}

- (void)webView:(WKWebView*)webView
    didFinishNavigation:(WKNavigation*)navigation {
  __weak WalletWebViewRunnerDelegate* weakSelf = self;
  [webView evaluateJavaScript:@"localStorage.clear()"
            completionHandler:^(id result, NSError* error) {
              [weakSelf done];
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

class WalletWebViewRunner {
 public:
  WalletWebViewRunner() = default;

  ~WalletWebViewRunner() = default;

  void Clear(ProfileIOS* profile) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    WKWebViewConfiguration* config =
        web::WKWebViewConfigurationProvider::FromBrowserState(profile)
            .GetWebViewConfiguration();
    web_view_ = [[WKWebView alloc] initWithFrame:CGRectZero
                                   configuration:config];
    delegate_ = [[WalletWebViewRunnerDelegate alloc]
        initWithClosure:base::BindOnce(&WalletWebViewRunner::OnDone,
                                       weak_factory_.GetWeakPtr())];
    web_view_.navigationDelegate = delegate_;
    [web_view_ loadRequest:[NSURLRequest
                               requestWithURL:
                                   [NSURL URLWithString:@(kBraveUIWalletURL)]]];
  }

 private:
  void OnDone() {
    web_view_ = nil;
    delegate_ = nil;
  }

  WKWebView* web_view_ = nil;
  WalletWebViewRunnerDelegate* delegate_ = nil;
  base::WeakPtrFactory<WalletWebViewRunner> weak_factory_{this};
};

class BraveWalletServiceDelegateIos : public BraveWalletServiceDelegate {
 public:
  explicit BraveWalletServiceDelegateIos(ProfileIOS* profile)
      : profile_(profile), cleaner_runner_(web::GetUIThreadTaskRunner({})) {
    wallet_base_directory_ = profile->GetStatePath();
    is_private_window_ = profile->IsOffTheRecord();
  }

  void ClearWalletUIStoragePartition() override {
    cleaner_runner_.AsyncCall(&WalletWebViewRunner::Clear)
        .WithArgs(profile_.get());
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
  base::SequenceBound<WalletWebViewRunner> cleaner_runner_;
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

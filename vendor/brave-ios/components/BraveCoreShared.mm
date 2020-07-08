#include "brave/vendor/brave-ios/components/BraveCoreShared.h"

#include "base/files/file_path.h"
#include "base/mac/bundle_locations.h"
#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/app/brave_main_delegate.h"
#import "brave/ios/browser/brave_web_client.h"
#include "brave/ios/browser/browser_state/browser_state_manager.h"
#include "components/bookmarks/browser/startup_task_runner_service.h"
#include "ios/chrome/browser/bookmarks/startup_task_runner_service_factory.h"
#include "ios/chrome/app/startup/provider_registration.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#include "ios/web/public/init/web_main.h"

@interface BraveCoreShared()
@property (nonatomic, assign) NSString*(^fetchUserAgent)();
@end

@interface BraveCoreShared()
{
    std::unique_ptr<BraveWebClient> web_client_;
    std::unique_ptr<BraveMainDelegate> delegate_;
    std::unique_ptr<web::WebMain> web_main_;
}
@end

@implementation BraveCoreShared

+ (instancetype)shared {
    static BraveCoreShared* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[BraveCoreShared alloc] init];
    });
    return instance;
}

- (instancetype)init {
    if ((self = [super init])) {

        // TODO(bridiver) - this should probably go in BraveMainDelegate
        base::FilePath path;
        base::PathService::Get(base::DIR_MODULE, &path);
        base::mac::SetOverrideFrameworkBundlePath(path);

        // Register all providers before calling any Chromium code.
        [ProviderRegistration registerProviders];

        web_client_.reset(new BraveWebClient());
        web::SetWebClient(web_client_.get());

        delegate_.reset(new BraveMainDelegate());

        web::WebMainParams params(delegate_.get());

        web_main_ = std::make_unique<web::WebMain>(std::move(params));

        ios::GetChromeBrowserProvider()->Initialize();

        auto* browser_state =
            BrowserStateManager::GetInstance().GetBrowserState();

        ios::StartupTaskRunnerServiceFactory::GetForBrowserState(browser_state)
            ->StartDeferredTaskRunners();
    }
    return self;
}

- (void)dealloc {
    web_main_.reset();
    delegate_.reset();
    web_client_.reset();
}

- (NSString *)getUserAgent {
    if (self.fetchUserAgent) {
        return self.fetchUserAgent();
    }
    return @"";
}

- (void)setUserAgentCallback:(NSString*(^)())userAgentCallback {
    self.fetchUserAgent = userAgentCallback;
}

@end

#include "brave/vendor/brave-ios/components/BraveCoreShared.h"

#include "base/files/file_path.h"
#include "base/mac/bundle_locations.h"
#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/app/brave_main_delegate.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"
#include "ios/web/public/init/web_main.h"

#import "base/i18n/icu_util.h"
#import "base/ios/ios_util.h"

@interface BraveCoreShared()
@property (nonatomic, assign) NSString*(^fetchUserAgent)();
@end

@interface BraveCoreShared()
{
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

        base::FilePath path;
        base::PathService::Get(base::DIR_MODULE, &path);
        base::mac::SetOverrideFrameworkBundlePath(path);

        delegate_.reset(new BraveMainDelegate());

        web::WebMainParams params(delegate_.get());

        web_main_ = std::make_unique<web::WebMain>(std::move(params));

        /*web::ShellWebClient* client =
            static_cast<web::ShellWebClient*>(web::GetWebClient());
        web::BrowserState* browserState = client->browser_state();*/
    }
    return self;
}

- (void)dealloc {
    web_main_.reset();
    delegate_.reset();
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

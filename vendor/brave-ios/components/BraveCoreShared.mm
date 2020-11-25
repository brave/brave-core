#include "brave/vendor/brave-ios/components/BraveCoreShared.h"
#include "brave/ios/web/brave_webmain.h"

#include "base/files/file_path.h"
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
    std::unique_ptr<web::BraveWebMain> web_main_;
    std::unique_ptr<ChromeBrowserState> browser_state_;
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
        
        const auto pathToICUDTL = [[NSBundle bundleForClass:NSClassFromString(@"BATBraveLedger")] pathForResource:@"icudtl" ofType:@"dat"];
        base::ios::OverridePathOfEmbeddedICU(pathToICUDTL.UTF8String);
        if (!base::i18n::InitializeICU()) {
          //BLOG(0, @"Failed to initialize ICU data");
        }
        
        delegate_.reset(new BraveMainDelegate());

        web::WebMainParams params(delegate_.get());
        params.register_exit_manager = false;
        
        web_main_ = std::make_unique<web::BraveWebMain>(std::move(params));
        
        /*web::ShellWebClient* client =
            static_cast<web::ShellWebClient*>(web::GetWebClient());
        web::BrowserState* browserState = client->browser_state();*/

        browser_state_ = std::make_unique<ChromeBrowserState>(
            base::FilePath(kIOSChromeInitialBrowserState));
    }
    return self;
}

- (void)dealloc {
    browser_state_.reset();
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

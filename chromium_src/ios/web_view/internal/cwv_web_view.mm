#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "ios/chrome/browser/tabs/model/tab_helper_util.h"

#include "src/ios/web_view/internal/cwv_web_view.mm"

@implementation CWVWebView (Extras)

- (void)updateScripts {
  // This runs `UpdateScripts` on the configuration provider which we will need
  // to call in-place of `-[WKUserContentController removeAllUserScripts]` until
  // all Brave JavaScript features are ported over to actual Chromium
  // JavascriptFeature types and added to BraveWebClient
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(
          _webState->GetBrowserState());
  config_provider.UpdateScripts();
}

- (void)createPDF:(void (^)(NSData* _Nullable))completionHandler {
  _webState->CreateFullPagePdf(base::BindOnce(completionHandler));
}

- (void)takeSnapshotWithRect:(CGRect)rect
           completionHandler:(void (^)(UIImage* _Nullable))completionHandler {
  _webState->TakeSnapshot(rect, base::BindRepeating(completionHandler));
}

@end

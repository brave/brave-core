#include "brave/ios/browser/api/ipfs/ipfs_api.h"

#include "brave/components/ipfs/ipfs_utils.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

@implementation IpfsAPI {
  ChromeBrowserState* _mainBrowserState;  // NOT OWNED
}

- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState {
  if ((self = [super init])) {
    _mainBrowserState = mainBrowserState;
  }
  return self;
}

- (NSURL*)resolveGatewayUrlFor:(NSString*)input {
  auto* browserState = _mainBrowserState;
  GURL input_gurl = net::GURLWithNSURL([NSURL URLWithString:input]);
  GURL output;
  PrefService* prefs = user_prefs::UserPrefs::Get(browserState);
  auto gateway_url = ipfs::GetDefaultNFTIPFSGateway(prefs);

  ipfs::TranslateIPFSURI(input_gurl, &output, gateway_url, false);
  return net::NSURLWithGURL(output);
}

- (NSURL*)getNftIPFSGateway {
  auto* browserState = _mainBrowserState;
  PrefService* prefs = user_prefs::UserPrefs::Get(browserState);
  auto gateway = ipfs::GetDefaultNFTIPFSGateway(prefs);
  return net::NSURLWithGURL(gateway);
}

- (void)setNftIPFSGateway:(NSString*)input {
  auto* browserState = _mainBrowserState;
  PrefService* prefs = user_prefs::UserPrefs::Get(browserState);
  GURL input_gurl = net::GURLWithNSURL([NSURL URLWithString:input]);
  ipfs::SetDefaultNFTIPFSGateway(prefs, input_gurl);
}

@end

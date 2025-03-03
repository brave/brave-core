#include "src/ios/chrome/app/startup/client_registration.mm"

@implementation ClientRegistration(Brave)
+ (void)registerClientsBrave {
  // Register CookieStoreIOSClient, This is used to provide CookieStoreIOSClient
  // users with WEB::IO task runner.
  net::SetCookieStoreIOSClient(new ChromeCookieStoreIOSClient());
}
@end

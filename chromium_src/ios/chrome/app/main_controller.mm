#include "brave/ios/app/brave_ios_main.h"
#include "ios/chrome/app/startup/ios_chrome_main.h"
#include "ios/chrome/app/startup/chrome_main_starter.h"
#include "ios/chrome/app/startup/client_registration.h"
#include "ios/chrome/app/variations_app_state_agent.h"

@interface ChromeMainStarter(Brave)
+ (std::unique_ptr<BraveIOSMain>)startBraveMain;
@end

@implementation ChromeMainStarter(Brave)
+ (std::unique_ptr<BraveIOSMain>)startBraveMain {
  return std::make_unique<BraveIOSMain>();
}
@end

#define IOSChromeMain BraveIOSMain
#define startChromeMain startBraveMain
#define registerClients registerClientsBrave
#define VariationsAppStateAgent BraveVariationsAppStateAgent

#include "src/ios/chrome/app/main_controller.mm"

#undef VariationsAppStateAgent
#undef registerClients
#undef startChromeMain
#undef IOSChromeMain

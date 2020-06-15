#include "brave/ios/web/brave_webmain.h"
#include "brave/ios/web/brave_webmain_runner.h"

#include "ios/web/public/init/web_main.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

BraveWebMain::BraveWebMain(WebMainParams params) {
  web_main_runner_.reset(BraveWebMainRunner::Create());
  web_main_runner_->Initialize(std::move(params));
}

BraveWebMain::~BraveWebMain() {
  web_main_runner_->ShutDown();
}

}  // namespace web

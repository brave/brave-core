#import "brave/ios/app/brave_ios_main.h"

#import <UIKit/UIKit.h>

#import <vector>

#import "base/check.h"
#import "base/strings/sys_string_conversions.h"
#import "base/time/time.h"
#import "ios/web/public/init/web_main.h"

namespace {
base::TimeTicks* g_start_time;
}  // namespace

BraveIOSMain::BraveIOSMain() {
  web::WebMainParams main_params(&main_delegate_);
  
  // base::CommandLine should already be setup
  // WebMain called CommandLine::Init(argc, argv)
  // which will do nothing since it's already setup
  main_params.argc = 0;
  main_params.argv = nullptr;

  // Chrome registers an AtExitManager in main in order to initialize the crash
  // handler early, so prevent a second registration by WebMainRunner.
  main_params.register_exit_manager = false;
  web_main_ = std::make_unique<web::WebMain>(std::move(main_params));
}

BraveIOSMain::~BraveIOSMain() {}

// static
void BraveIOSMain::InitStartTime() {
  DCHECK(!g_start_time);
  g_start_time = new base::TimeTicks(base::TimeTicks::Now());
}

// static
const base::TimeTicks& BraveIOSMain::StartTime() {
  CHECK(g_start_time);
  return *g_start_time;
}

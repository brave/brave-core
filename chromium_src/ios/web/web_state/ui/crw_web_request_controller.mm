
#include "ios/web/web_state/web_state_impl.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"

// Replace WebUI handler with Brave's
#define CreateWebUI CreateBraveWebUI
#define ClearWebUI ClearBraveWebUI
#define HasWebUI HasBraveWebUI

#include "src/ios/web/web_state/ui/crw_web_request_controller.mm"

#undef HasWebUI
#undef ClearWebUI
#undef CreateWebUI

#pragma clang diagnostic pop

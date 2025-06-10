
#include "ios/web/web_state/web_state_impl.h"

// Redirect WebUI messages to Brave's handler
#define HandleWebUIMessage HandleBraveWebUIMessage

#include "src/ios/web/webui/web_ui_messaging_java_script_feature.mm"

#undef HandleWebUIMessage

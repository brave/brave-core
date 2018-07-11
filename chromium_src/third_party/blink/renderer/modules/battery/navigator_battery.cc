#include "third_party/blink/renderer/modules/battery/navigator_battery.h"

#include "gin/converter.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_isolate_data.h"
#include "net/proxy_resolution/proxy_resolver_v8.h"

namespace blink {
  ScriptPromise NavigatorBattery::getBattery(ScriptState* script_state,
                                           Navigator& navigator) {
    v8::Isolate* isolate = script_state->GetIsolate();
    return blink::ScriptPromise::Reject(script_state,
           v8::Exception::TypeError(gin::StringToV8(isolate, "This api has been disabled.")));
  }
} // namespace blink

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_BRAVE_DEVTOOLS_SINK_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_BRAVE_DEVTOOLS_SINK_H_

#include "brave/third_party/blink/public/web/web_brave_devtools.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class WebBraveDevtoolsSink final
    : public GarbageCollected<WebBraveDevtoolsSink>,
      public Supplement<LocalFrame> {
 public:
  static constexpr const char kSupplementName[] = "WebBraveDevtoolsSink";

  explicit WebBraveDevtoolsSink(LocalFrame& frame);
  ~WebBraveDevtoolsSink();

  static WebBraveDevtoolsSink* From(LocalFrame* frame);

  // probes:
  void BraveDevtoolsEnabled(bool enabled);
  void BraveDevtoolsMessageReceived(const WebString& message,
                                    const base::Value::Dict& params);

  void AddWebBraveDevtoolsClient(WebBraveDevtoolsClient* client);
  void RemoveWebBraveDevtoolsClient(WebBraveDevtoolsClient* client);

 private:
  WTF::Vector<WebBraveDevtoolsClient*> web_brave_devtools_clients_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_BRAVE_DEVTOOLS_SINK_H_

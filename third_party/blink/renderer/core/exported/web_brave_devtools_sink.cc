
#include "brave/third_party/blink/renderer/core/exported/web_brave_devtools_sink.h"

#include "third_party/blink/renderer/core/probe/core_probes.h"

namespace blink {

WebBraveDevtoolsSink::WebBraveDevtoolsSink(LocalFrame& frame)
    : Supplement<LocalFrame>(frame) {
  GetSupplementable()->GetProbeSink()->AddWebBraveDevtoolsSink(this);
}

WebBraveDevtoolsSink::~WebBraveDevtoolsSink() {
  GetSupplementable()->GetProbeSink()->RemoveWebBraveDevtoolsSink(this);
}

// static
WebBraveDevtoolsSink* WebBraveDevtoolsSink::From(LocalFrame* frame) {
  if (!frame) {
    return nullptr;
  }

  auto* sink = Supplement<LocalFrame>::From<WebBraveDevtoolsSink>(*frame);
  if (sink) {
    return sink;
  }
  sink = MakeGarbageCollected<WebBraveDevtoolsSink>(*frame);
  Supplement<LocalFrame>::ProvideTo(*frame, sink);
  return sink;
}

void WebBraveDevtoolsSink::AddWebBraveDevtoolsClient(
    WebBraveDevtoolsClient* client) {
  web_brave_devtools_clients_.push_back(client);
}

void WebBraveDevtoolsSink::RemoveWebBraveDevtoolsClient(
    WebBraveDevtoolsClient* client) {
  const auto fnd = web_brave_devtools_clients_.Find(client);
  if (fnd != WTF::kNotFound) {
    web_brave_devtools_clients_.EraseAt(fnd);
  }
}

void WebBraveDevtoolsSink::BraveDevtoolsEnabled(bool enabled) {
  for (auto* client : web_brave_devtools_clients_) {
    client->BraveDevtoolsEnabled(enabled);
  }
}

void WebBraveDevtoolsSink::BraveDevtoolsMessageReceived(
    const WebString& message,
    const base::Value::Dict& params) {
  for (auto* client : web_brave_devtools_clients_) {
    client->HandleBraveDevtoolsMessage(message, params);
  }
}

}  // namespace blink

#ifndef BRAVE_RENDERER_BRAVE_DEBUGGER_API_H_
#define BRAVE_RENDERER_BRAVE_DEBUGGER_API_H_

#include <string>

#include "brave/common/brave_debugger.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

// JavaScript API for brave debugger functionality
// Exposes window.debugger object when puppeteer permissions are granted
class BraveDebuggerAPI : public gin::Wrappable<BraveDebuggerAPI> {
 public:
  static constexpr gin::WrapperInfo kWrapperInfo = {{gin::kEmbedderNativeGin},
                                                    gin::kBraveDebugger};
  static void Install(content::RenderFrame* frame, v8::Local<v8::Context> context);
  static bool ShouldInject(content::RenderFrame* frame, v8::Local<v8::Context> context);

  explicit BraveDebuggerAPI(content::RenderFrame* frame);

  BraveDebuggerAPI(const BraveDebuggerAPI&) = delete;
  BraveDebuggerAPI& operator=(const BraveDebuggerAPI&) = delete;

  // JavaScript API methods (matching chrome.debugger)
  v8::Local<v8::Promise> attach(gin::Arguments* args);
  v8::Local<v8::Promise> detach(gin::Arguments* args);
  v8::Local<v8::Promise> sendCommand(gin::Arguments* args);
  v8::Local<v8::Promise> getTargets(gin::Arguments* args);

  ~BraveDebuggerAPI() override;

 protected:

 private:
  // gin::Wrappable implementation
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;

  // Get or create the mojo remote connection
  brave::mojom::BraveDebugger* GetDebuggerInterface();

  // Helpers to convert between V8 and base::Value
  base::Value::Dict V8ObjectToDictionary(v8::Isolate* isolate, v8::Local<v8::Object> object);
  v8::Local<v8::Object> DictionaryToV8Object(v8::Isolate* isolate, const base::Value::Dict& dict);

  raw_ptr<content::RenderFrame> render_frame_;
  mojo::Remote<brave::mojom::BraveDebugger> debugger_remote_;

  base::WeakPtrFactory<BraveDebuggerAPI> weak_factory_{this};
};

#endif  // BRAVE_RENDERER_BRAVE_DEBUGGER_API_H_
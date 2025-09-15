#include "brave/renderer/brave_debugger_api.h"

#include "base/values.h"
#include "brave/components/permissions/contexts/brave_puppeteer_permission_context.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "gin/converter.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/origin.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-promise.h"


// static
void BraveDebuggerAPI::Install(content::RenderFrame* frame,
                              v8::Local<v8::Context> context) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> debugger_api;

  BraveDebuggerAPI* handler = cppgc::MakeGarbageCollected<BraveDebuggerAPI>(
      isolate->GetCppHeap()->GetAllocationHandle(), frame);
  v8::Local<v8::Value> handler_value;
  if (gin::Converter<BraveDebuggerAPI*>::ToV8(isolate, handler).ToLocal(&handler_value) &&
      handler_value->ToObject(context).ToLocal(&debugger_api)) {
    global->Set(context, gin::StringToV8(isolate, "debugger"), debugger_api).Check();
  }
}

// static
bool BraveDebuggerAPI::ShouldInject(content::RenderFrame* frame,
                                   v8::Local<v8::Context> context) {
  if (!frame || !frame->GetWebFrame()) {
    return false;
  }

  // Check if this is the main frame (not a subframe)
  if (!frame->GetWebFrame()->IsOutermostMainFrame()) {
    return false;
  }

  // Get the origin and check if it has puppeteer permissions
  blink::WebSecurityOrigin web_origin = frame->GetWebFrame()->GetSecurityOrigin();
  url::Origin origin = web_origin;

  // For now, we'll inject the API and let the permission check happen at usage time
  // In a full implementation, we might want to check permissions here
  return true;
}

BraveDebuggerAPI::BraveDebuggerAPI(content::RenderFrame* frame)
    : render_frame_(frame) {}

BraveDebuggerAPI::~BraveDebuggerAPI() = default;

const gin::WrapperInfo* BraveDebuggerAPI::wrapper_info() const {
  return &kWrapperInfo;
}

gin::ObjectTemplateBuilder BraveDebuggerAPI::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<BraveDebuggerAPI>::GetObjectTemplateBuilder(isolate)
      .SetMethod("attach", &BraveDebuggerAPI::attach)
      .SetMethod("detach", &BraveDebuggerAPI::detach)
      .SetMethod("sendCommand", &BraveDebuggerAPI::sendCommand)
      .SetMethod("getTargets", &BraveDebuggerAPI::getTargets);
}

v8::Local<v8::Promise> BraveDebuggerAPI::attach(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Promise::Resolver> resolver;

  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) {
    return v8::Local<v8::Promise>();
  }

  // Parse arguments: attach({tabId: number}, version)
  v8::Local<v8::Object> target_obj;
  std::string version = "1.3";  // default version

  if (args->Length() >= 1 && args->GetNext(&target_obj)) {
    if (args->Length() >= 2) {
      args->GetNext(&version);
    }

    // Extract target ID from the target object
    v8::Local<v8::Value> tab_id_value;
    if (target_obj->Get(context, gin::StringToV8(isolate, "tabId")).ToLocal(&tab_id_value)) {
      std::string target_id;
      if (gin::Converter<std::string>::FromV8(isolate, tab_id_value, &target_id)) {

        // Get the origin
        blink::WebSecurityOrigin web_origin = render_frame_->GetWebFrame()->GetSecurityOrigin();
        url::Origin origin = web_origin;

        auto* debugger_interface = GetDebuggerInterface();
        if (debugger_interface) {
          debugger_interface->AttachToTarget(
              origin, target_id, version,
              base::BindOnce(
                  [](v8::Global<v8::Promise::Resolver> resolver_global,
                     v8::Global<v8::Context> context_global,
                     v8::Isolate* isolate, bool success, const std::string& error) {
                    v8::HandleScope handle_scope(isolate);
                    v8::Local<v8::Context> context = context_global.Get(isolate);
                    v8::Context::Scope context_scope(context);
                    v8::MicrotasksScope microtasks_scope(context, v8::MicrotasksScope::kDoNotRunMicrotasks);
                    v8::Local<v8::Promise::Resolver> resolver = resolver_global.Get(isolate);

                    if (success) {
                      resolver->Resolve(context, v8::Undefined(isolate)).Check();
                    } else {
                      v8::Local<v8::String> error_message = gin::StringToV8(isolate, error);
                      resolver->Reject(context, error_message).Check();
                    }
                  },
                  v8::Global<v8::Promise::Resolver>(isolate, resolver),
                  v8::Global<v8::Context>(isolate, context), isolate));
        } else {
          resolver->Reject(context, gin::StringToV8(isolate, "Debugger interface not available")).Check();
        }
      } else {
        resolver->Reject(context, gin::StringToV8(isolate, "Invalid target ID")).Check();
      }
    } else {
      resolver->Reject(context, gin::StringToV8(isolate, "Missing tabId in target")).Check();
    }
  } else {
    resolver->Reject(context, gin::StringToV8(isolate, "Invalid arguments")).Check();
  }

  return resolver->GetPromise();
}

v8::Local<v8::Promise> BraveDebuggerAPI::detach(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Promise::Resolver> resolver;

  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) {
    return v8::Local<v8::Promise>();
  }

  // Parse arguments: detach({tabId: number})
  v8::Local<v8::Object> target_obj;

  if (args->Length() >= 1 && args->GetNext(&target_obj)) {
    v8::Local<v8::Value> tab_id_value;
    if (target_obj->Get(context, gin::StringToV8(isolate, "tabId")).ToLocal(&tab_id_value)) {
      std::string target_id;
      if (gin::Converter<std::string>::FromV8(isolate, tab_id_value, &target_id)) {

        // Get the origin
        blink::WebSecurityOrigin web_origin = render_frame_->GetWebFrame()->GetSecurityOrigin();
        url::Origin origin = web_origin;

        auto* debugger_interface = GetDebuggerInterface();
        if (debugger_interface) {
          debugger_interface->DetachFromTarget(
              origin, target_id,
              base::BindOnce(
                  [](v8::Global<v8::Promise::Resolver> resolver_global,
                     v8::Global<v8::Context> context_global,
                     v8::Isolate* isolate, bool success, const std::string& error) {
                    v8::HandleScope handle_scope(isolate);
                    v8::Local<v8::Context> context = context_global.Get(isolate);
                    v8::Context::Scope context_scope(context);
                    v8::MicrotasksScope microtasks_scope(context, v8::MicrotasksScope::kDoNotRunMicrotasks);
                    v8::Local<v8::Promise::Resolver> resolver = resolver_global.Get(isolate);

                    if (success) {
                      resolver->Resolve(context, v8::Undefined(isolate)).Check();
                    } else {
                      v8::Local<v8::String> error_message = gin::StringToV8(isolate, error);
                      resolver->Reject(context, error_message).Check();
                    }
                  },
                  v8::Global<v8::Promise::Resolver>(isolate, resolver),
                  v8::Global<v8::Context>(isolate, context), isolate));
        } else {
          resolver->Reject(context, gin::StringToV8(isolate, "Debugger interface not available")).Check();
        }
      } else {
        resolver->Reject(context, gin::StringToV8(isolate, "Invalid target ID")).Check();
      }
    } else {
      resolver->Reject(context, gin::StringToV8(isolate, "Missing tabId in target")).Check();
    }
  } else {
    resolver->Reject(context, gin::StringToV8(isolate, "Invalid arguments")).Check();
  }

  return resolver->GetPromise();
}

v8::Local<v8::Promise> BraveDebuggerAPI::sendCommand(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Promise::Resolver> resolver;

  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) {
    return v8::Local<v8::Promise>();
  }

  // Parse arguments: sendCommand({tabId: number}, method, params)
  v8::Local<v8::Object> target_obj;
  std::string method;
  v8::Local<v8::Object> params_obj;

  if (args->Length() >= 2 && args->GetNext(&target_obj) && args->GetNext(&method)) {
    base::Value::Dict params;
    if (args->Length() >= 3 && args->GetNext(&params_obj)) {
      params = V8ObjectToDictionary(isolate, params_obj);
    }

    v8::Local<v8::Value> tab_id_value;
    if (target_obj->Get(context, gin::StringToV8(isolate, "tabId")).ToLocal(&tab_id_value)) {
      std::string target_id;
      if (gin::Converter<std::string>::FromV8(isolate, tab_id_value, &target_id)) {

        // Get the origin
        blink::WebSecurityOrigin web_origin = render_frame_->GetWebFrame()->GetSecurityOrigin();
        url::Origin origin = web_origin;

        auto* debugger_interface = GetDebuggerInterface();
        if (debugger_interface) {
          debugger_interface->SendCommand(
              origin, target_id, method, std::move(params),
              base::BindOnce(
                  [](v8::Global<v8::Promise::Resolver> resolver_global,
                     v8::Global<v8::Context> context_global,
                     v8::Isolate* isolate, bool success,
                     base::Value::Dict result, const std::string& error) {
                    v8::HandleScope handle_scope(isolate);
                    v8::Local<v8::Context> context = context_global.Get(isolate);
                    v8::Context::Scope context_scope(context);
                    v8::MicrotasksScope microtasks_scope(context, v8::MicrotasksScope::kDoNotRunMicrotasks);
                    v8::Local<v8::Promise::Resolver> resolver = resolver_global.Get(isolate);

                    if (success) {
                      // Convert result dictionary to V8 object using V8ValueConverter
                      auto converter = content::V8ValueConverter::Create();
                      v8::Local<v8::Value> result_value = converter->ToV8Value(base::ValueView(result), context);
                      resolver->Resolve(context, result_value).Check();
                    } else {
                      v8::Local<v8::String> error_message = gin::StringToV8(isolate, error);
                      resolver->Reject(context, error_message).Check();
                    }
                  },
                  v8::Global<v8::Promise::Resolver>(isolate, resolver),
                  v8::Global<v8::Context>(isolate, context), isolate));
        } else {
          resolver->Reject(context, gin::StringToV8(isolate, "Debugger interface not available")).Check();
        }
      } else {
        resolver->Reject(context, gin::StringToV8(isolate, "Invalid target ID")).Check();
      }
    } else {
      resolver->Reject(context, gin::StringToV8(isolate, "Missing tabId in target")).Check();
    }
  } else {
    resolver->Reject(context, gin::StringToV8(isolate, "Invalid arguments")).Check();
  }

  return resolver->GetPromise();
}

v8::Local<v8::Promise> BraveDebuggerAPI::getTargets(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Promise::Resolver> resolver;

  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) {
    return v8::Local<v8::Promise>();
  }

  // Get the origin
  blink::WebSecurityOrigin web_origin = render_frame_->GetWebFrame()->GetSecurityOrigin();
  url::Origin origin = web_origin;

  auto* debugger_interface = GetDebuggerInterface();
  if (debugger_interface) {
    debugger_interface->GetTargets(
        origin,
        base::BindOnce(
            [](v8::Global<v8::Promise::Resolver> resolver_global,
               v8::Global<v8::Context> context_global,
               v8::Isolate* isolate,
               std::vector<brave::mojom::DebuggerTargetPtr> targets) {
              v8::HandleScope handle_scope(isolate);
              v8::Local<v8::Context> context = context_global.Get(isolate);
              v8::Context::Scope context_scope(context);
              v8::MicrotasksScope microtasks_scope(context, v8::MicrotasksScope::kDoNotRunMicrotasks);
              v8::Local<v8::Promise::Resolver> resolver = resolver_global.Get(isolate);

              v8::Local<v8::Array> targets_array = v8::Array::New(isolate, targets.size());
              for (size_t i = 0; i < targets.size(); ++i) {
                const auto& target = targets[i];
                v8::Local<v8::Object> target_obj = v8::Object::New(isolate);

                target_obj->Set(context, gin::StringToV8(isolate, "id"),
                               gin::StringToV8(isolate, target->id)).Check();
                target_obj->Set(context, gin::StringToV8(isolate, "type"),
                               gin::StringToV8(isolate, target->type)).Check();
                target_obj->Set(context, gin::StringToV8(isolate, "title"),
                               gin::StringToV8(isolate, target->title)).Check();
                target_obj->Set(context, gin::StringToV8(isolate, "url"),
                               gin::StringToV8(isolate, target->url)).Check();
                target_obj->Set(context, gin::StringToV8(isolate, "attached"),
                               v8::Boolean::New(isolate, target->attached)).Check();

                targets_array->Set(context, i, target_obj).Check();
              }

              resolver->Resolve(context, targets_array).Check();
            },
            v8::Global<v8::Promise::Resolver>(isolate, resolver),
            v8::Global<v8::Context>(isolate, context), isolate));
  } else {
    resolver->Reject(context, gin::StringToV8(isolate, "Debugger interface not available")).Check();
  }

  return resolver->GetPromise();
}

brave::mojom::BraveDebugger* BraveDebuggerAPI::GetDebuggerInterface() {
  if (!debugger_remote_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker().GetInterface(
        debugger_remote_.BindNewPipeAndPassReceiver());
  }
  return debugger_remote_.get();
}

base::Value::Dict BraveDebuggerAPI::V8ObjectToDictionary(v8::Isolate* isolate,
                                                        v8::Local<v8::Object> object) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  auto converter = content::V8ValueConverter::Create();

  auto base_value = converter->FromV8Value(object, context);
  if (base_value && base_value->is_dict()) {
    return std::move(*base_value).TakeDict();
  }

  return base::Value::Dict();
}

v8::Local<v8::Object> BraveDebuggerAPI::DictionaryToV8Object(v8::Isolate* isolate,
                                                            const base::Value::Dict& dict) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  auto converter = content::V8ValueConverter::Create();

  v8::Local<v8::Value> v8_value = converter->ToV8Value(base::ValueView(dict), context);
  if (v8_value->IsObject()) {
    return v8_value.As<v8::Object>();
  }

  return v8::Object::New(isolate);
}
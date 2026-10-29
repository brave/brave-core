/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/renderer/psst_js_handler.h"

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/psst/core/common/constants.h"
#include "brave/gin/converter_specializations.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-script.h"

namespace psst {

namespace {

// Wraps the native object (`$1`, i.e. `kPsstNativeObjectName`) in the API the
// scripts actually use:
//
//   psstLog(level, message, data)
//
// and forwards `console.log/info/warn/error/debug` to it, while still calling
// through to the original so DevTools keeps working. Both are defined
// non-configurably so a script can't accidentally clobber them.
constexpr char kBootstrapScript[] = R"JS(
(function() {
  const native = globalThis[$1];
  if (!native || globalThis.psstLog) {
    return;
  }

  const format = (value) => {
    if (typeof value === 'string') {
      return value;
    }
    if (value instanceof Error) {
      return value.stack || String(value);
    }
    try {
      const json = JSON.stringify(value);
      return json === undefined ? String(value) : json;
    } catch (e) {
      return String(value);
    }
  };

  const psstLog = (level, message, data) => {
    try {
      native.psstLog(String(level), format(message), data);
    } catch (e) {
      // Never let logging break the flow.
    }
  };

  Object.defineProperty(globalThis, 'psstLog', {
    value: psstLog,
    writable: false,
    enumerable: false,
    configurable: false,
  });

  const consoleLevels = {
    log: 'info',
    info: 'info',
    warn: 'warn',
    error: 'error',
    debug: 'debug',
  };

  for (const [name, level] of Object.entries(consoleLevels)) {
    const original = console[name];
    Object.defineProperty(console, name, {
      value: function(...args) {
        psstLog(level, args.map(format).join(' '));
        if (typeof original === 'function') {
          return original.apply(console, args);
        }
      },
      writable: false,
      enumerable: false,
      configurable: false,
    });
  }
})();
)JS";

mojom::PsstLogLevel LevelFromString(std::string_view level) {
  static constexpr auto kLevels =
      base::MakeFixedFlatMap<std::string_view, mojom::PsstLogLevel>({
          {"debug", mojom::PsstLogLevel::kDebug},
          {"info", mojom::PsstLogLevel::kInfo},
          {"warn", mojom::PsstLogLevel::kWarn},
          {"error", mojom::PsstLogLevel::kError},
      });

  // Unrecognized levels are still logged, just as info.
  const auto* mapped_level = base::FindOrNull(kLevels, level);
  return mapped_level ? *mapped_level : mojom::PsstLogLevel::kInfo;
}

// Runs `kBootstrapScript` in `context`, swallowing any error so a failure to
// install logging can't break script injection.
void RunBootstrapScript(v8::Isolate* isolate, v8::Local<v8::Context> context) {
  const std::string source = base::ReplaceStringPlaceholders(
      kBootstrapScript, {base::StrCat({"'", kPsstNativeObjectName, "'"})},
      nullptr);

  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Script> script;
  if (!v8::Script::Compile(context, gin::StringToV8(isolate, source))
           .ToLocal(&script)) {
    return;
  }
  std::ignore = script->Run(context);
}

}  // namespace

PsstJSHandler::PsstJSHandler(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

PsstJSHandler::~PsstJSHandler() = default;

// static
// The script-facing contract is documented in the "Script logging" section of
// brave/components/psst/README.md.
void PsstJSHandler::Install(content::RenderFrame* render_frame,
                            v8::Local<v8::Context> context) {
  CHECK(render_frame);
  // No context to install into, e.g. a frame being torn down.
  if (context.IsEmpty()) {
    return;
  }

  v8::Isolate* isolate =
      render_frame->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  // Already installed for this context.
  v8::Local<v8::Value> existing;
  if (!global->Get(context, gin::StringToV8(isolate, kPsstNativeObjectName))
           .ToLocal(&existing) ||
      !existing->IsUndefined()) {
    return;
  }

  PsstJSHandler* handler = cppgc::MakeGarbageCollected<PsstJSHandler>(
      isolate->GetCppHeap()->GetAllocationHandle(), render_frame);

  v8::PropertyDescriptor desc(handler->GetWrapper(isolate).ToLocalChecked(),
                              false);
  desc.set_configurable(false);
  desc.set_enumerable(false);

  // Without the native object the bootstrap script has nothing to wrap, so
  // skip it rather than leaving a half-installed API behind.
  if (!global
           ->DefineProperty(
               context, gin::StringToV8(isolate, kPsstNativeObjectName), desc)
           .IsJust()) {
    return;
  }

  RunBootstrapScript(isolate, context);
}

void PsstJSHandler::OnDestruct() {}

gin::ObjectTemplateBuilder PsstJSHandler::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<PsstJSHandler>::GetObjectTemplateBuilder(isolate)
      .SetMethod("psstLog", &PsstJSHandler::PsstLog);
}

const gin::WrapperInfo* PsstJSHandler::wrapper_info() const {
  return &kWrapperInfo;
}

void PsstJSHandler::PsstLog(gin::Arguments* args) {
  if (!render_frame()) {
    return;
  }

  std::string level;
  std::string message;
  if (!args->GetNext(&level) || !args->GetNext(&message)) {
    args->ThrowTypeError("psstLog requires a level and a message");
    return;
  }

  // `data` is optional; anything that isn't a plain object is ignored.
  std::optional<base::DictValue> data;
  if (!args->PeekNext().IsEmpty()) {
    base::DictValue dict;
    if (args->GetNext(&dict)) {
      data = std::move(dict);
    }
  }

  auto& log_sink = GetLogSink();
  if (log_sink.is_bound()) {
    log_sink->Log(LevelFromString(level), message, std::move(data));
  }
}

mojo::AssociatedRemote<mojom::PsstLogSink>& PsstJSHandler::GetLogSink() {
  if (!log_sink_.is_bound()) {
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&log_sink_);
    log_sink_.reset_on_disconnect();
  }
  return log_sink_;
}

}  // namespace psst

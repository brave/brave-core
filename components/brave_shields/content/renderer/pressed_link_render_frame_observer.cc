// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/renderer/pressed_link_render_frame_observer.h"

#include <iterator>
#include <tuple>

#include "base/functional/bind.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/gurl.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-script.h"

namespace brave_shields {

namespace {

// Evaluates to a function that takes the function to call with each link the
// user presses. composedPath() reaches links in open shadow roots, and click
// is needed because a link opened from the keyboard only fires click.
constexpr char kLinkPressListenerScript[] =
    R"((onLinkPressed) => {
          const onPress = (event) => {
            if (!event.isTrusted) {
              return;
            }

            for (const node of event.composedPath()) {
              if (node instanceof Element &&
                  node.matches('a[href], area[href]')) {
                if (typeof node.href === 'string') {
                  onLinkPressed(node.href);
                }

                return;
              }
            }
          };
          document.addEventListener('mousedown', onPress, true);
          document.addEventListener('click', onPress, true);
        })";

}  // namespace

PressedLinkRenderFrameObserver::PressedLinkRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t isolated_world_id)
    : content::RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<PressedLinkRenderFrameObserver>(
          render_frame),
      isolated_world_id_(isolated_world_id) {}

PressedLinkRenderFrameObserver::~PressedLinkRenderFrameObserver() = default;

void PressedLinkRenderFrameObserver::OnDestruct() {
  delete this;
}

void PressedLinkRenderFrameObserver::RunScriptsAtDocumentStart() {
  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  v8::Isolate* isolate = web_frame->GetAgentGroupScheduler()->Isolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      web_frame->GetScriptContextFromWorldId(isolate, isolated_world_id_);

  if (context.IsEmpty()) {
    return;
  }

  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(
      context, v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Script> script;
  v8::Local<v8::Value> install;

  if (!v8::Script::Compile(context,
                           gin::StringToV8(isolate, kLinkPressListenerScript))
           .ToLocal(&script) ||
      !script->Run(context).ToLocal(&install) || !install->IsFunction()) {
    return;
  }

  v8::Local<v8::Function> on_link_pressed;

  if (!gin::CreateFunctionTemplate(
           isolate,
           base::BindRepeating(&PressedLinkRenderFrameObserver::OnLinkPressed,
                               weak_ptr_factory_.GetWeakPtr()))
           ->GetFunction(context)
           .ToLocal(&on_link_pressed)) {
    return;
  }

  v8::Local<v8::Value> args[] = {on_link_pressed};
  std::ignore = install.As<v8::Function>()->Call(context, context->Global(),
                                                 std::size(args), args);
}

void PressedLinkRenderFrameObserver::OnLinkPressed(const std::string& url) {
  if (!brave_shields_host_.is_bound()) {
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
        &brave_shields_host_);
    brave_shields_host_.reset_on_disconnect();
  }

  brave_shields_host_->OnLinkPressed(GURL(url));
}

}  // namespace brave_shields

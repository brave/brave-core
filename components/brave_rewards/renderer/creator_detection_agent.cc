// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_rewards/renderer/creator_detection_agent.h"

#include <array>
#include <optional>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/publisher_utils.h"
#include "brave/components/brave_rewards/resources/grit/creator_detection_generated.h"
#include "content/public/renderer/render_frame.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace brave_rewards {

namespace {

constexpr auto kScriptMap = base::MakeFixedFlatMap<std::string_view, int>(
    {{"github.com", IDR_CREATOR_DETECTION_GITHUB_BUNDLE_JS},
     {"www.github.com", IDR_CREATOR_DETECTION_GITHUB_BUNDLE_JS},
     {"gist.github.com", IDR_CREATOR_DETECTION_GITHUB_BUNDLE_JS},
     {"reddit.com", IDR_CREATOR_DETECTION_REDDIT_BUNDLE_JS},
     {"www.reddit.com", IDR_CREATOR_DETECTION_REDDIT_BUNDLE_JS},
     {"twitch.tv", IDR_CREATOR_DETECTION_TWITCH_BUNDLE_JS},
     {"www.twitch.tv", IDR_CREATOR_DETECTION_TWITCH_BUNDLE_JS},
     {"twitter.com", IDR_CREATOR_DETECTION_TWITTER_BUNDLE_JS},
     {"x.com", IDR_CREATOR_DETECTION_TWITTER_BUNDLE_JS},
     {"vimeo.com", IDR_CREATOR_DETECTION_VIMEO_BUNDLE_JS},
     {"www.youtube.com", IDR_CREATOR_DETECTION_YOUTUBE_BUNDLE_JS},
     {"m.youtube.com", IDR_CREATOR_DETECTION_YOUTUBE_BUNDLE_JS}});

bool ShouldRunInIsolatedWorld(int resource_id) {
  // By default, scripts are loaded into an isolated JS world. However, the
  // Twitter script needs to access JS objects that are set directly on DOM
  // nodes in the main world.
  if (resource_id == IDR_CREATOR_DETECTION_TWITTER_BUNDLE_JS) {
    return false;
  }
  return true;
}

v8::Isolate* GetIsolate(content::RenderFrame* render_frame) {
  DCHECK(render_frame);
  return render_frame->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
}

struct DetectionScript {
  std::string source;
  bool should_run_in_isolated_world = true;
};

std::optional<DetectionScript> GetDetectionScript(
    content::RenderFrame* render_frame) {
  // Only run scripts for the main frame.
  if (!render_frame || !render_frame->IsMainFrame()) {
    return std::nullopt;
  }

  // Only run scripts if the user has enabled Brave Rewards.
  if (!render_frame->GetBlinkPreferences().brave_rewards_enabled) {
    return std::nullopt;
  }

  auto* web_frame = render_frame->GetWebFrame();
  DCHECK(web_frame);
  if (web_frame->IsProvisional()) {
    return std::nullopt;
  }

  // Only run scripts for known "media platform" sites.
  GURL origin_url = url::Origin(web_frame->GetSecurityOrigin()).GetURL();
  if (!IsMediaPlatformURL(origin_url)) {
    return std::nullopt;
  }

  // Only run scripts when there is an exact hostname match.
  auto iter = kScriptMap.find(origin_url.host_piece());
  if (iter == kScriptMap.end()) {
    return std::nullopt;
  }

  return DetectionScript{
      .source = blink::Platform::Current()->GetDataResourceString(iter->second),
      .should_run_in_isolated_world = ShouldRunInIsolatedWorld(iter->second)};
}

v8::MaybeLocal<v8::Function> CompileDetectionInitializer(
    v8::Local<v8::Context> context,
    std::string_view function_body) {
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(
      context, v8::MicrotasksScope::kDoNotRunMicrotasks);

  auto* isolate = context->GetIsolate();
  v8::ScriptCompiler::Source source(gin::StringToV8(isolate, function_body));

  v8::Local<v8::String> parameters[] = {
      gin::StringToV8(isolate, "setPageChangedCallback"),
      gin::StringToV8(isolate, "onCreatorDetected"),
      gin::StringToV8(isolate, "verboseLogging")};

  // The creator detection script is compiled as a function body. Browser
  // capabilities are passed in to the compiled function as arguments whose
  // parameter names are defined above. Browser capabilities are only exposed
  // to the script; they are not visible outside of the compiled function.
  return v8::ScriptCompiler::CompileFunction(context, &source, 3, parameters);
}

template <typename... Args>
void CallFunctionWithArgs(v8::Isolate* isolate,
                          v8::Local<v8::Function> function,
                          Args... args) {
  v8::Local<v8::Context> context = function->GetCreationContextChecked(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(
      context, v8::MicrotasksScope::kDoNotRunMicrotasks);
  std::array<v8::Local<v8::Value>, sizeof...(Args)> arg_list = {
      args.template As<v8::Value>()...};
  std::ignore = function->Call(context, context->Global(), arg_list.size(),
                               arg_list.empty() ? nullptr : arg_list.data());
}

}  // namespace

CreatorDetectionAgent::CreatorDetectionAgent(content::RenderFrame* render_frame,
                                             int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      isolated_world_id_(isolated_world_id) {}

CreatorDetectionAgent::~CreatorDetectionAgent() = default;

void CreatorDetectionAgent::DidCommitProvisionalLoad(
    ui::PageTransition transition) {
  MaybeInjectDetectionScript();
}

void CreatorDetectionAgent::DidFinishSameDocumentNavigation() {
  MaybeNotifyPageChanged();
}

void CreatorDetectionAgent::WillReleaseScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (page_changed_callback_.IsEmpty()) {
    return;
  }
  // If we have a page changed callback, and its creation context is the context
  // that is being released, then release the persistent handle to the callback.
  auto* isolate = GetIsolate(render_frame());
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  auto function = page_changed_callback_.Get(isolate);
  if (function->GetCreationContextChecked(isolate) == context) {
    page_changed_callback_.Reset();
  }
}

void CreatorDetectionAgent::OnDestruct() {
  delete this;
}

mojo::AssociatedRemote<mojom::CreatorDetectionHost>&
CreatorDetectionAgent::GetDetectionHost() {
  if (!detection_host_) {
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
        &detection_host_);
    detection_host_.reset_on_disconnect();
  }

  return detection_host_;
}

void CreatorDetectionAgent::MaybeInjectDetectionScript() {
  if (!page_changed_callback_.IsEmpty()) {
    return;
  }

  auto script = GetDetectionScript(render_frame());
  if (!script) {
    return;
  }

  auto* isolate = GetIsolate(render_frame());
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context;
  if (script->should_run_in_isolated_world) {
    context = render_frame()->GetWebFrame()->GetScriptContextFromWorldId(
        isolate, isolated_world_id_);
  } else {
    context = render_frame()->GetWebFrame()->MainWorldScriptContext();
  }

  if (context.IsEmpty()) {
    return;
  }

  auto initializer = CompileDetectionInitializer(context, script->source);
  if (initializer.IsEmpty()) {
    return;
  }

  auto js_callback = [&](auto fn) {
    auto function_template = gin::CreateFunctionTemplate(
        isolate, base::BindRepeating(fn, weak_factory_.GetWeakPtr()));
    return function_template->GetFunction(context).ToLocalChecked();
  };

  CallFunctionWithArgs(
      isolate, initializer.ToLocalChecked(),
      js_callback(&CreatorDetectionAgent::SetPageChangedCallback),
      js_callback(&CreatorDetectionAgent::OnCreatorDetected),
      v8::Boolean::New(isolate, base::FeatureList::IsEnabled(
                                    features::kVerboseLoggingFeature)));

  DCHECK(!page_changed_callback_.IsEmpty());

  MaybeNotifyPageChanged();
}

void CreatorDetectionAgent::MaybeNotifyPageChanged() {
  if (page_changed_callback_.IsEmpty()) {
    return;
  }

  auto* isolate = GetIsolate(render_frame());
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);

  CallFunctionWithArgs(isolate, page_changed_callback_.Get(isolate));
}

void CreatorDetectionAgent::SetPageChangedCallback(gin::Arguments* args) {
  v8::HandleScope handle_scope(args->isolate());
  v8::Local<v8::Function> callback;
  if (!args->GetNext(&callback)) {
    args->ThrowError();
    return;
  }
  page_changed_callback_.Reset(args->isolate(), callback);
}

void CreatorDetectionAgent::OnCreatorDetected(const std::string& id,
                                              const std::string& name,
                                              const std::string& url,
                                              const std::string& image_url) {
  GetDetectionHost()->OnCreatorDetected(id, name, url, image_url);
}

}  // namespace brave_rewards

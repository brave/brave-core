/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include "base/functional/bind.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace playlist {

PlaylistRenderFrameObserver::PlaylistRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<PlaylistRenderFrameObserver>(render_frame),
      isolated_world_id_(isolated_world_id) {
  EnsureConnectedToMediaHandler();
}

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() = default;

void PlaylistRenderFrameObserver::OnDestruct() {
  delete this;
}

bool PlaylistRenderFrameObserver::EnsureConnectedToMediaHandler() {
  if (!media_handler_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker()->GetInterface(
        media_handler_.BindNewPipeAndPassReceiver());
    media_handler_.set_disconnect_handler(
        base::BindOnce(&PlaylistRenderFrameObserver::OnMediaHandlerDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return media_handler_.is_bound();
}

void PlaylistRenderFrameObserver::OnMediaHandlerDisconnect() {
  media_handler_.reset();
  EnsureConnectedToMediaHandler();
}

void PlaylistRenderFrameObserver::RunScriptsAtDocumentStart() {
  if (render_frame()->GetWebFrame()->IsProvisional()) {
    return;
  }

  const auto& blink_preferences = render_frame()->GetBlinkPreferences();
  if (blink_preferences.hide_media_src_api) {
    HideMediaSourceAPI();
  }

  if (blink_preferences.should_detect_media_files) {
    InstallMediaDetector();
  }
}

// Disables the MediaSource API in hope of the page switching to
// network-fetchable HTTPS URLs. This script is from
// https://github.com/brave/brave-ios/blob/development/Sources/Brave/Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/PlaylistSwizzlerScript.js
void PlaylistRenderFrameObserver::HideMediaSourceAPI() const {
  DVLOG(2) << __FUNCTION__;

  render_frame()->GetWebFrame()->ExecuteScript(
      blink::WebScriptSource(blink::WebString::FromASCII(R"(
    (function() {
      if (
        window.MediaSource ||
        window.WebKitMediaSource ||
        window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId
      ) {
        delete window.MediaSource;
        delete window.WebKitMediaSource;
      }
    })();)")));
}

void PlaylistRenderFrameObserver::InstallMediaDetector() {
  DVLOG(2) << __FUNCTION__;

  static const char kScript[] = R"(
    (function(onMediaUpdated) {
      // Firstly, we try to get find all <video> or <audio> tags periodically,
      // for a a while from the start up. If we find them, then we attach 
      // MutationObservers to them to detect source URL.
      // After a given amount of time, we do this in requestIdleCallback().
      // Note that there's a global object named |pl_worker|. This worker is
      // created and bound by PlaylistJSHandler.

      const mutationSources = new Set();
      const mutationObserver = new MutationObserver(mutations => {
        mutations.forEach(mutation => { onMediaUpdated(mutation.target.src); })
      });
      const findNewMediaAndObserveMutation = () => {
          return document.querySelectorAll('video, audio').forEach((mediaNode) => {
              if (mutationSources.has(mediaNode)) return

              mutationSources.add(mediaNode)
              onMediaUpdated(mediaNode.src)
              mutationObserver.observe(mediaNode, { attributeFilter: ['src'] })
          });
      }

      const pollingIntervalId = window.setInterval(findNewMediaAndObserveMutation, 1000);
      window.setTimeout(() => {
          window.clearInterval(pollingIntervalId)
          window.requestIdleCallback(findNewMediaAndObserveMutation)
          // TODO(sko) We might want to check if idle callback is waiting too long.
          // In that case, we should get back to the polling style. And also, this
          // time could be too long for production.
      }, 20000)
    })
  )";

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame()->GetWebFrame()->GetScriptContextFromWorldId(
          isolate, isolated_world_id_);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(
      context, v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Script> script =
      v8::Script::Compile(context, gin::StringToV8(isolate, kScript))
          .ToLocalChecked();
  v8::Local<v8::Function> function =
      v8::Local<v8::Function>::Cast(script->Run(context).ToLocalChecked());

  v8::Local<v8::Function> on_media_updated =
      gin::CreateFunctionTemplate(
          context->GetIsolate(),
          base::BindRepeating(&PlaylistRenderFrameObserver::OnMediaUpdated,
                              weak_ptr_factory_.GetWeakPtr()))
          ->GetFunction(context)
          .ToLocalChecked();
  v8::Local<v8::Value> arg = on_media_updated.As<v8::Value>();

  std::ignore = function->Call(context, context->Global(), 1, &arg);
}

void PlaylistRenderFrameObserver::OnMediaUpdated(const std::string& page_url) {
  if (!GURL(page_url).SchemeIsHTTPOrHTTPS()) {
    return;
  }

  if (!EnsureConnectedToMediaHandler()) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " " << page_url;

  media_handler_->OnMediaUpdatedFromRenderFrame();
}

}  // namespace playlist

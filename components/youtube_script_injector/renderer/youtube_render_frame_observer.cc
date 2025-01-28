/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 #include "brave/components/youtube_script_injector/renderer/youtube_render_frame_observer.h"
 #include "brave/components/youtube_script_injector/browser/core/youtube_registry.h"
 #include "brave/components/youtube_script_injector/browser/content/youtube_tab_helper.h"

 #include "content/public/renderer/render_frame.h"
 #include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
 #include "third_party/blink/public/platform/web_isolated_world_info.h"
 #include "third_party/blink/public/platform/web_url.h"
 #include "third_party/blink/public/web/web_local_frame.h"
 #include "v8/include/v8.h"

 namespace youtube_script_injector {

 YouTubeRenderFrameObserver::YouTubeRenderFrameObserver(
     content::RenderFrame* render_frame,
     int32_t world_id)
     : RenderFrameObserver(render_frame), world_id_(world_id) {
 }

 YouTubeRenderFrameObserver::~YouTubeRenderFrameObserver() = default;

 void YouTubeRenderFrameObserver::DidCreateScriptContext(
     v8::Local<v8::Context> context,
     int32_t world_id) {
 }

 void YouTubeRenderFrameObserver::DidStartNavigation(const GURL& url,
   std::optional<blink::WebNavigationType> navigation_type) {
   url_ = url;
}

 void YouTubeRenderFrameObserver::DidFinishLoad() {
   if (!render_frame()->IsMainFrame()) {
      return;
   }

   if (!YouTubeRegistry::IsYouTubeDomain(url_)) {
      return;
   }

   blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
   v8::Isolate* isolate = web_frame->GetAgentGroupScheduler()->Isolate();

    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
    if (context.IsEmpty()) {
        return;
    }

   int32_t world_id =  web_frame->GetScriptContextWorldId(context);
   if (world_id_ != world_id) {
      return;
   }

   if (!native_javascript_handle_) {
     native_javascript_handle_ = std::make_unique<YouTubeInjectorFrameJSHandler>(
         render_frame());
   } else {
     native_javascript_handle_->ResetRemote(render_frame());
   }

   native_javascript_handle_->AddJavaScriptObjectToFrame(context);
 }

 void YouTubeRenderFrameObserver::OnDestruct() {
   delete this;
 }

 }  // namespace youtube_script_injector

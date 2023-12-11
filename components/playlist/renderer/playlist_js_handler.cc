/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_js_handler.h"

#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/json/values_util.h"
#include "content/public/renderer/render_frame.h"
#include "content/renderer/v8_value_converter_impl.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace {

template <typename Signature>
void BindFunctionToObject(v8::Isolate* isolate,
                          v8::Local<v8::Object> javascript_object,
                          const std::string& function_name,
                          const base::RepeatingCallback<Signature>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Get the isolate associated with this object.
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, function_name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

}  // namespace

namespace playlist {

PlaylistJSHandler::PlaylistJSHandler(content::RenderFrame* render_frame,
                                     const int32_t isolated_world_id)
    : render_frame_(render_frame), isolated_world_id_(isolated_world_id) {
  EnsureConnectedToMediaHandler();
}

PlaylistJSHandler::~PlaylistJSHandler() {}

void PlaylistJSHandler::AddWorkerObjectToFrame(v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty()) {
    return;
  }

  CreateWorkerObject(isolate, context);
}

void PlaylistJSHandler::SetDetectorScript(const GURL& url,
                                          const blink::WebString& script) {
  url_ = url;
  script_ = script;
}

bool PlaylistJSHandler::EnsureConnectedToMediaHandler() {
  if (!media_handler_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        media_handler_.BindNewPipeAndPassReceiver());
    media_handler_.set_disconnect_handler(
        base::BindOnce(&PlaylistJSHandler::OnMediaHandlerDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return media_handler_.is_bound();
}

void PlaylistJSHandler::OnMediaHandlerDisconnect() {
  media_handler_.reset();
  EnsureConnectedToMediaHandler();
}

void PlaylistJSHandler::CreateWorkerObject(v8::Isolate* isolate,
                                           v8::Local<v8::Context> context) {
  DVLOG(2) << __FUNCTION__;
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> pl_worker;
  if (!global->Get(context, gin::StringToV8(isolate, "pl_worker"))
           .ToLocal(&pl_worker) ||
      !pl_worker->IsObject()) {
    v8::Local<v8::Object> pl_worker_object;
    pl_worker_object = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "pl_worker"),
              pl_worker_object)
        .Check();
    BindFunctionsToWorkerObject(isolate, pl_worker_object);
  }
}

void PlaylistJSHandler::BindFunctionsToWorkerObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> worker_object) {
  DVLOG(2) << __FUNCTION__;
  BindFunctionToObject(isolate, worker_object, "onMediaUpdated",
                       base::BindRepeating(&PlaylistJSHandler::OnMediaUpdated,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void PlaylistJSHandler::OnMediaUpdated(const std::string& src) {
  DVLOG(2) << __FUNCTION__ << " " << src;
  if (!EnsureConnectedToMediaHandler()) {
    return;
  }

  auto* web_frame = render_frame_->GetWebFrame();
  WTF::Vector sources = {blink::WebScriptSource(script_)};
  web_frame->RequestExecuteScript(
      isolated_world_id_, sources,
      blink::mojom::UserActivationOption::kActivate,
      blink::mojom::EvaluationTiming::kAsynchronous,
      blink::mojom::LoadEventBlockingOption::kBlock,
      base::BindOnce(&PlaylistJSHandler::OnFindMedia,
                     weak_ptr_factory_.GetWeakPtr(), url_),
      blink::BackForwardCacheAware::kAllow,
      blink::mojom::WantResultOption::kWantResult,
      blink::mojom::PromiseResultOption::kAwait);
}

void PlaylistJSHandler::OnFindMedia(GURL requested_url,
                                    absl::optional<base::Value> value,
                                    base::TimeTicks time_ticks) {
  if (!value) {
    LOG(ERROR) << *value;
    return;
  }
  /* Expected output:
[
  {
    "detected": boolean,
    "mimeType": "video" | "audio",
    "name": string,
    "pageSrc": url,
    "pageTitle": string
    "src": url
    "thumbnail": url | undefined
  }
]
*/

  if (value->is_dict() && value->GetDict().empty()) {
    DVLOG(2) << "No media was detected";
    return;
  }

  if (!value->is_list()) {
    LOG(ERROR) << __func__
               << " Got invalid value after running media detector script "
               << (int)value->type();
    return;
  }

  std::vector<mojom::PlaylistItemPtr> items;
  for (const auto& media : value->GetList()) {
    if (!media.is_dict()) {
      LOG(ERROR) << __func__ << " Got invalid item";
      continue;
    }

    const auto& media_dict = media.GetDict();

    auto* name = media_dict.FindString("name");
    auto* page_title = media_dict.FindString("pageTitle");
    auto* page_source = media_dict.FindString("pageSrc");
    auto* mime_type = media_dict.FindString("mimeType");
    auto* src = media_dict.FindString("src");
    if (!name || !page_source || !page_title || !mime_type || !src) {
      LOG(ERROR) << __func__ << " required fields are not satisfied";
      continue;
    }

    // nullable data
    auto* thumbnail = media_dict.FindString("thumbnail");
    auto* author = media_dict.FindString("author");
    auto duration = media_dict.FindDouble("duration");

    auto item = mojom::PlaylistItem::New();
    item->id = base::Token::CreateRandom().ToString();
    item->page_source = requested_url;
    item->page_redirected = GURL(*page_source);
    item->name = *name;
    // URL data
    if (GURL media_url(*src);
        !media_url.SchemeIs(url::kHttpsScheme) && !media_url.SchemeIsBlob()) {
      LOG(ERROR) << __func__ << "media scheme is not https://";
      continue;
    }

    if (thumbnail) {
      if (GURL thumbnail_url(*thumbnail);
          !thumbnail_url.SchemeIs(url::kHttpsScheme)) {
        LOG(ERROR) << __func__ << "thumbnail scheme is not https://";
        thumbnail = nullptr;
      }
    }

    if (duration.has_value()) {
      item->duration =
          base::TimeDeltaToValue(base::Seconds(*duration)).GetString();
    }
    if (thumbnail) {
      item->thumbnail_source = GURL(*thumbnail);
      item->thumbnail_path = GURL(*thumbnail);
    }
    item->media_source = GURL(*src);
    item->media_path = GURL(*src);
    if (author) {
      item->author = *author;
    }

    items.push_back(std::move(item));
  }
  media_handler_->OnMediaUpdatedFromRenderFrame({}, std::move(items));
}

}  // namespace playlist

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_COSMETIC_FILTERS_CONTENT_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_
#define BRAVE_COMPONENTS_COSMETIC_FILTERS_CONTENT_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_

#include <string>

#include "brave/content/browser/mojom/cosmetic_filters_communication.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

namespace cosmetic_filters_worker {

class CosmeticFiltersJSHandler {
 public:
  explicit CosmeticFiltersJSHandler(content::RenderFrame* render_frame);
  ~CosmeticFiltersJSHandler();

  // Adds the "cs_worker" JavaScript object and its functions to the current
  // |RenderFrame|.
  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);

 private:
  // Add a function to the provided object.
  template <typename Sig>
  void BindFunctionToObject(v8::Isolate* isolate,
      v8::Local<v8::Object> javascript_object, const std::string& name,
      const base::RepeatingCallback<Sig>& callback);
  void EnsureConnected();

  // A function to be called from JS
  void HiddenClassIdSelectors(const std::string& input);

  content::RenderFrame* render_frame_;
  mojo::Remote<cf_comm::mojom::CosmeticFiltersCommunication> cs_communicator_;
};

// static
v8::Local<v8::Object> GetOrCreateWorkerObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context);

}  // namespace cosmetic_filters_worker

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_CONTENT_RENDERER_COSMETIC_FILTERS_JS_HANDLER_H_

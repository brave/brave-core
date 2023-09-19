// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SCRIPT_INJECTOR_RENDERER_SCRIPT_INJECTOR_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_SCRIPT_INJECTOR_RENDERER_SCRIPT_INJECTOR_RENDER_FRAME_OBSERVER_H_

#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"

namespace script_injector {

/**
 * This class is responsible for injecting async scripts into the renderer from
 * the browser process, and sending the result back to the browser process.
 */
class COMPONENT_EXPORT(SCRIPT_INJECTOR) ScriptInjectorRenderFrameObserver
    : public content::RenderFrameObserver,
      public mojom::ScriptInjector {
 public:
  ScriptInjectorRenderFrameObserver();
  ScriptInjectorRenderFrameObserver(content::RenderFrame* render_frame);
  ScriptInjectorRenderFrameObserver(const ScriptInjectorRenderFrameObserver&) =
      delete;
  ScriptInjectorRenderFrameObserver& operator=(
      const ScriptInjectorRenderFrameObserver&) = delete;
  ~ScriptInjectorRenderFrameObserver() override;

  // Implement the RequestAsyncExecuteScript method.
  void RequestAsyncExecuteScript(
      int32_t world_id,
      const std::u16string& script,
      blink::mojom::UserActivationOption user_activation,
      blink::mojom::PromiseResultOption await_promise,
      RequestAsyncExecuteScriptCallback callback) override;

 private:
  // RenderFrameObserver implementation:
  void OnDestruct() override;

  void BindToReceiver(
      mojo::PendingAssociatedReceiver<script_injector::mojom::ScriptInjector>
          pending_receiver);

  mojo::AssociatedReceiverSet<script_injector::mojom::ScriptInjector>
      receivers_;

  base::WeakPtrFactory<ScriptInjectorRenderFrameObserver> weak_ptr_factory_;
};

}  // namespace script_injector

#endif  // BRAVE_COMPONENTS_SCRIPT_INJECTOR_RENDERER_SCRIPT_INJECTOR_RENDER_FRAME_OBSERVER_H_

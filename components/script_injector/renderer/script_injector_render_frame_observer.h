// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SCRIPT_INJECTOR_RENDERER_SCRIPT_INJECTOR_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_SCRIPT_INJECTOR_RENDERER_SCRIPT_INJECTOR_RENDER_FRAME_OBSERVER_H_

#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"

#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"

namespace script_injector {

// This class is responsible for injecting async scripts into the renderer from
// the browser process, and sending the result back to the browser process.
class COMPONENT_EXPORT(SCRIPT_INJECTOR_RENDERER)
    ScriptInjectorRenderFrameObserver : public content::RenderFrameObserver,
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

  // Determines whether the caller should be sent a result back.
  // Tested in ScriptInjectorUnitTest.
  static blink::mojom::WantResultOption CheckIfWantResult(
      const RequestAsyncExecuteScriptCallback& callback);

  // Used to bind the mojo receiver to the render frame observer.
  void BindToReceiver(
      mojo::PendingAssociatedReceiver<script_injector::mojom::ScriptInjector>
          pending_receiver);

  // We maintain a list of receivers, since there might be multiple ones.
  mojo::AssociatedReceiverSet<script_injector::mojom::ScriptInjector>
      receivers_;

  base::WeakPtrFactory<ScriptInjectorRenderFrameObserver> weak_ptr_factory_;

  FRIEND_TEST_ALL_PREFIXES(ScriptInjectorUnitTest,
                           CheckIfWantResultHasCallback);
  FRIEND_TEST_ALL_PREFIXES(ScriptInjectorUnitTest,
                           CheckIfWantResultNullCallback);
  friend class ScriptInjectorUnitTest;
};

}  // namespace script_injector

#endif  // BRAVE_COMPONENTS_SCRIPT_INJECTOR_RENDERER_SCRIPT_INJECTOR_RENDER_FRAME_OBSERVER_H_

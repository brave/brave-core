// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_AI_SERVICE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_AI_SERVICE_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace local_ai {

// LocalAIService manages the lifecycle of background model workers.
//
// Consumers call GetPassageEmbedder() to obtain a PassageEmbedder remote
// bound directly to the content layer (BackgroundWebContentsImpl). The
// service itself does not proxy inference calls — it only manages
// creation/teardown of the background WebContents and forwards the
// renderer's PassageEmbedder registration to it.
class LocalAIService : public KeyedService,
                       public mojom::LocalAIService,
                       public BackgroundWebContents::Delegate {
 public:
  // Factory that creates a platform-specific BackgroundWebContents. Platform
  // params (BrowserContext*, URL, tagging callback) are bound into the
  // closure at the browser layer.
  using BackgroundWebContentsFactory =
      base::RepeatingCallback<std::unique_ptr<BackgroundWebContents>(
          BackgroundWebContents::Delegate* delegate)>;

  explicit LocalAIService(BackgroundWebContentsFactory factory);
  ~LocalAIService() override;

  LocalAIService(const LocalAIService&) = delete;
  LocalAIService& operator=(const LocalAIService&) = delete;

  mojo::PendingRemote<mojom::LocalAIService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::LocalAIService> receiver);

  // mojom::LocalAIService:
  void RegisterPassageEmbedder(
      mojo::PendingRemote<mojom::PassageEmbedder> embedder) override;
  void GetPassageEmbedder(GetPassageEmbedderCallback callback) override;

 private:
  // KeyedService:
  void Shutdown() override;

  // BackgroundWebContents::Delegate:
  void OnBackgroundContentsReady() override;
  void OnBackgroundContentsDestroyed(
      BackgroundWebContents::DestroyReason reason) override;

  void MaybeCreateBackgroundContents();
  void CloseBackgroundContents();

  std::unique_ptr<BackgroundWebContents> background_web_contents_;
  BackgroundWebContentsFactory background_web_contents_factory_;
  mojo::ReceiverSet<mojom::LocalAIService> receivers_;

  base::WeakPtrFactory<LocalAIService> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_AI_SERVICE_H_

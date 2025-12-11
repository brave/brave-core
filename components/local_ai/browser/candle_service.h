// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/local_ai/common/candle.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class WebContents;
}

namespace local_ai {

class CandleService : public KeyedService,
                      public mojom::CandleService,
                      public content::WebContentsObserver,
                      public ProfileObserver {
 public:
  explicit CandleService(Profile* profile);
  ~CandleService() override;

  CandleService(const CandleService&) = delete;
  CandleService& operator=(const CandleService&) = delete;

  void BindReceiver(mojo::PendingReceiver<mojom::CandleService> receiver);

  void BindEmbeddingGemma(
      mojo::PendingRemote<mojom::EmbeddingGemmaInterface>) override;

  void GetDefaultModelPath(GetDefaultModelPathCallback callback) override;

  void LoadModelFiles(const base::FilePath& weights_path,
                      const base::FilePath& tokenizer_path,
                      const base::FilePath& config_path,
                      LoadModelFilesCallback callback) override;

  void Embed(const std::string& text, EmbedCallback callback) override;

 private:
  // content::WebContentsObserver:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

  // KeyedService:
  void Shutdown() override;

  void OnEmbeddingGemmaModelFilesLoaded(LoadModelFilesCallback callback,
                                        mojom::ModelFilesPtr model_files);

  void LoadWasmModel();
  void OnGotDefaultModelPath(const std::optional<base::FilePath>& model_path);
  void OnModelFilesLoaded(bool success);
  void RetryLoadWasmModel();
  void CloseWasmWebContents();

  raw_ptr<Profile> profile_ = nullptr;

  // The single WebContents that loads the WASM and maintains the model
  std::unique_ptr<content::WebContents> wasm_web_contents_;

  mojo::ReceiverSet<mojom::CandleService> receivers_;

  // Single embedder remote (shared by all callers)
  mojo::Remote<mojom::EmbeddingGemmaInterface> embedding_gemma_remote_;

  // Model loading state
  base::FilePath pending_model_path_;
  int model_load_retry_count_ = 0;
  static constexpr int kMaxModelLoadRetries = 10;

  base::WeakPtrFactory<CandleService> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_

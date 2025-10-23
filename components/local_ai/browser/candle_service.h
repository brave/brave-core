// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "brave/components/local_ai/common/candle.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace local_ai {

class CandleService : public mojom::CandleService {
 public:
  static CandleService* GetInstance();

  CandleService(const CandleService&) = delete;
  CandleService& operator=(const CandleService&) = delete;

  void BindReceiver(mojo::PendingReceiver<mojom::CandleService> receiver);

  void BindBert(mojo::PendingRemote<mojom::BertInterface>) override;

  void BindEmbeddingGemma(
      mojo::PendingRemote<mojom::EmbeddingGemmaInterface>) override;

  void BindPhi(mojo::PendingRemote<mojom::PhiInterface>) override;

  void Embed(const std::string& text, EmbedCallback callback) override;

  void RunBertExample();
  void RunEmbeddingGemmaInit();
  void RunPhiInit();

 private:
  friend class base::NoDestructor<CandleService>;

  CandleService();
  ~CandleService() override;

  void OnRunBertExample(const std::string& result);
  void OnEmbeddingGemmaModelFilesLoaded(mojom::LargeModelFilesPtr model_files);
  void OnEmbeddingGemmaInit(bool success);
  void OnPhiModelFilesLoaded(mojom::LargeModelFilesPtr model_files);
  void OnPhiInit(bool success);

  mojo::ReceiverSet<mojom::CandleService> receivers_;
  mojo::Remote<mojom::BertInterface> bert_remote_;

  mojo::Remote<mojom::EmbeddingGemmaInterface> embedding_gemma_remote_;

  mojo::Remote<mojom::PhiInterface> phi_remote_;

  base::WeakPtrFactory<CandleService> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_RUST_FFI_CANDLE_EMBEDDER_H_
#define BRAVE_COMPONENTS_LOCAL_AI_RUST_FFI_CANDLE_EMBEDDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/thread.h"

struct CCandleEmbedder;

namespace local_ai {

class CandleEmbedder {
 public:
  using InitCallback =
      base::OnceCallback<void(std::unique_ptr<CandleEmbedder> embedder,
                              const std::string& error_message)>;
  using EmbedCallback = base::OnceCallback<void(const std::vector<float>&)>;

  // Private token for factory construction pattern
  struct PrivateConstructorTag {
   private:
    friend class CandleEmbedder;
    PrivateConstructorTag() = default;
  };

  explicit CandleEmbedder(PrivateConstructorTag);
  ~CandleEmbedder();

  CandleEmbedder(const CandleEmbedder&) = delete;
  CandleEmbedder& operator=(const CandleEmbedder&) = delete;

  static void Create(std::vector<uint8_t> weights,
                     std::vector<uint8_t> weights_dense1,
                     std::vector<uint8_t> weights_dense2,
                     std::vector<uint8_t> tokenizer,
                     std::vector<uint8_t> config,
                     InitCallback callback);

  void Embed(const std::string& text, EmbedCallback callback);

 private:
  static void CreateOnRustThread(
      CandleEmbedder* wrapper,
      std::vector<uint8_t> weights,
      std::vector<uint8_t> weights_dense1,
      std::vector<uint8_t> weights_dense2,
      std::vector<uint8_t> tokenizer,
      std::vector<uint8_t> config,
      InitCallback callback,
      scoped_refptr<base::SequencedTaskRunner> origin_task_runner);

  static void OnInitCallback(void* user_data, bool success, const char* data);

  void EmbedOnRustThread(
      const std::string& text,
      EmbedCallback callback,
      scoped_refptr<base::SequencedTaskRunner> origin_task_runner);

  static void OnEmbedCallback(void* user_data,
                              const float* embeddings,
                              size_t length);

  struct InitCallbackData {
    InitCallbackData(
        raw_ptr<CandleEmbedder> wrapper,
        InitCallback callback,
        scoped_refptr<base::SequencedTaskRunner> origin_task_runner);
    ~InitCallbackData();

    raw_ptr<CandleEmbedder> wrapper;
    InitCallback callback;
    scoped_refptr<base::SequencedTaskRunner> origin_task_runner;
  };

  struct EmbedCallbackData {
    EmbedCallbackData(
        EmbedCallback callback,
        scoped_refptr<base::SequencedTaskRunner> origin_task_runner);
    ~EmbedCallbackData();

    EmbedCallback callback;
    scoped_refptr<base::SequencedTaskRunner> origin_task_runner;
  };

  raw_ptr<CCandleEmbedder> embedder_ = nullptr;
  base::Thread rust_thread_;

  base::WeakPtrFactory<CandleEmbedder> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_RUST_FFI_CANDLE_EMBEDDER_H_

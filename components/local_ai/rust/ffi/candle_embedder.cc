// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/rust/ffi/candle_embedder.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/local_ai/rust/ffi/candle_embedder_ffi.h"

namespace local_ai {

CandleEmbedder::CandleEmbedder() : rust_thread_("CandleRustThread") {
  base::Thread::Options options;
  rust_thread_.StartWithOptions(std::move(options));
}

CandleEmbedder::~CandleEmbedder() {
  if (embedder_) {
    rust_thread_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&candle_embedder_destroy, embedder_.get()));
    embedder_ = nullptr;
  }
  rust_thread_.Stop();
}

void CandleEmbedder::Create(std::vector<uint8_t> weights,
                            std::vector<uint8_t> weights_dense1,
                            std::vector<uint8_t> weights_dense2,
                            std::vector<uint8_t> tokenizer,
                            std::vector<uint8_t> config,
                            InitCallback callback) {
  auto origin_task_runner = base::SequencedTaskRunner::GetCurrentDefault();

  auto embedder = std::make_unique<CandleEmbedder>();
  auto* embedder_ptr = embedder.get();

  embedder_ptr->rust_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CandleEmbedder::CreateOnRustThread, embedder_ptr,
                     std::move(weights), std::move(weights_dense1),
                     std::move(weights_dense2), std::move(tokenizer),
                     std::move(config), std::move(callback),
                     origin_task_runner));

  embedder.release();
}

void CandleEmbedder::CreateOnRustThread(
    CandleEmbedder* wrapper,
    std::vector<uint8_t> weights,
    std::vector<uint8_t> weights_dense1,
    std::vector<uint8_t> weights_dense2,
    std::vector<uint8_t> tokenizer,
    std::vector<uint8_t> config,
    InitCallback callback,
    scoped_refptr<base::SequencedTaskRunner> origin_task_runner) {
  auto* callback_data =
      new InitCallbackData{wrapper, std::move(callback), origin_task_runner};

  candle_embedder_new(weights.data(), weights.size(), weights_dense1.data(),
                      weights_dense1.size(), weights_dense2.data(),
                      weights_dense2.size(), tokenizer.data(), tokenizer.size(),
                      config.data(), config.size(),
                      &CandleEmbedder::OnInitCallback, callback_data);
}

void CandleEmbedder::OnInitCallback(void* user_data,
                                    bool success,
                                    const char* data) {
  auto* callback_data = static_cast<InitCallbackData*>(user_data);

  if (success) {
    CCandleEmbedder* embedder =
        reinterpret_cast<CCandleEmbedder*>(const_cast<char*>(data));
    callback_data->wrapper->embedder_ = embedder;

    auto wrapper = base::WrapUnique(callback_data->wrapper.get());
    callback_data->origin_task_runner->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback_data->callback),
                                  std::move(wrapper), std::string()));
  } else {
    std::string error_message = data ? std::string(data) : "Unknown error";
    callback_data->origin_task_runner->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback_data->callback), nullptr,
                                  error_message));
    delete callback_data->wrapper;
  }

  delete callback_data;
}

void CandleEmbedder::Embed(const std::string& text, EmbedCallback callback) {
  auto origin_task_runner = base::SequencedTaskRunner::GetCurrentDefault();

  rust_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&CandleEmbedder::EmbedOnRustThread,
                                weak_ptr_factory_.GetWeakPtr(), text,
                                std::move(callback), origin_task_runner));
}

void CandleEmbedder::EmbedOnRustThread(
    const std::string& text,
    EmbedCallback callback,
    scoped_refptr<base::SequencedTaskRunner> origin_task_runner) {
  if (!embedder_) {
    std::vector<float> empty;
    origin_task_runner->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::ref(empty)));
    return;
  }

  auto* callback_data =
      new EmbedCallbackData{std::move(callback), origin_task_runner};

  candle_embedder_embed(embedder_.get(), text.c_str(),
                        &CandleEmbedder::OnEmbedCallback, callback_data);
}

void CandleEmbedder::OnEmbedCallback(void* user_data,
                                     const float* embeddings,
                                     size_t length) {
  auto* callback_data = static_cast<EmbedCallbackData*>(user_data);

  std::vector<float> result;
  if (embeddings && length > 0) {
    result.assign(embeddings, embeddings + length);
  }

  callback_data->origin_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback_data->callback), std::ref(result)));

  delete callback_data;
}

}  // namespace local_ai

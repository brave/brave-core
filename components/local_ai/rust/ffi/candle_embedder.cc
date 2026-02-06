// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/rust/ffi/candle_embedder.h"

#include <utility>

#include "base/containers/span.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/local_ai/rust/ffi/candle_embedder_ffi.h"

namespace local_ai {

CandleEmbedder::InitCallbackData::InitCallbackData(
    raw_ptr<CandleEmbedder> wrapper,
    InitCallback callback,
    scoped_refptr<base::SequencedTaskRunner> origin_task_runner)
    : wrapper(wrapper),
      callback(std::move(callback)),
      origin_task_runner(std::move(origin_task_runner)) {}

CandleEmbedder::InitCallbackData::~InitCallbackData() = default;

CandleEmbedder::EmbedCallbackData::EmbedCallbackData(
    EmbedCallback callback,
    scoped_refptr<base::SequencedTaskRunner> origin_task_runner,
    size_t text_length)
    : callback(std::move(callback)),
      origin_task_runner(std::move(origin_task_runner)),
      text_length(text_length) {}

CandleEmbedder::EmbedCallbackData::~EmbedCallbackData() = default;

CandleEmbedder::CandleEmbedder(PrivateConstructorTag)
    : rust_thread_("CandleRustThread") {
  base::Thread::Options options;
  rust_thread_.StartWithOptions(std::move(options));
}

CandleEmbedder::~CandleEmbedder() {
  if (embedder_) {
    rust_thread_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&candle_embedder_destroy, embedder_.get()));
    embedder_ = nullptr;
  }
  // Thread::Stop() calls PlatformThread::Join() which is a base sync primitive.
  // When destroyed via DeleteOnThreadPool during shutdown, thread pool workers
  // disallow sync primitives, so we must explicitly allow it here.
  base::ScopedAllowBaseSyncPrimitives allow_sync;
  rust_thread_.Stop();
}

// static
void CandleEmbedder::DeleteOnThreadPool(
    std::unique_ptr<CandleEmbedder> embedder) {
  if (!embedder) {
    return;
  }
  // Post deletion to a thread pool thread that allows blocking,
  // since ~CandleEmbedder() calls Thread::Stop() which blocks.
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce([](std::unique_ptr<CandleEmbedder> e) {},
                     std::move(embedder)));
}

void CandleEmbedder::Create(std::vector<uint8_t> weights,
                            std::vector<uint8_t> weights_dense1,
                            std::vector<uint8_t> weights_dense2,
                            std::vector<uint8_t> tokenizer,
                            std::vector<uint8_t> config,
                            InitCallback callback) {
  auto origin_task_runner = base::SequencedTaskRunner::GetCurrentDefault();

  auto embedder = std::make_unique<CandleEmbedder>(PrivateConstructorTag{});
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
      new InitCallbackData(wrapper, std::move(callback), origin_task_runner);

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
    // Delete on thread pool since we're on rust_thread_ and ~CandleEmbedder()
    // calls rust_thread_.Stop() which would deadlock if called from itself.
    DeleteOnThreadPool(base::WrapUnique(callback_data->wrapper.get()));
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
    origin_task_runner->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::vector<float>()));
    return;
  }

  auto* callback_data = new EmbedCallbackData(std::move(callback),
                                              origin_task_runner, text.size());

  candle_embedder_embed(embedder_.get(), text.c_str(),
                        &CandleEmbedder::OnEmbedCallback, callback_data);
}

void CandleEmbedder::OnEmbedCallback(void* user_data,
                                     const float* embeddings,
                                     size_t length) {
  auto* callback_data = static_cast<EmbedCallbackData*>(user_data);

  DVLOG(2) << "CandleEmbedder::Embed completed: text_length="
           << callback_data->text_length
           << ", time=" << callback_data->timer.Elapsed().InMilliseconds()
           << "ms, embedding_size=" << length;

  std::vector<float> result;
  if (embeddings && length > 0) {
    // SAFETY: embeddings points to a buffer of `length` floats from Rust FFI
    auto span = UNSAFE_BUFFERS(base::span(embeddings, length));
    result.assign(span.begin(), span.end());
  }

  callback_data->origin_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback_data->callback), std::move(result)));

  delete callback_data;
}

}  // namespace local_ai

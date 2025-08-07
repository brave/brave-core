/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 #include "brave/components/ai_chat/core/browser/text_embedder.h"

 #include <algorithm>
 #include <cmath>
 #include <ios>
 #include <ostream>
 #include <string_view>
 #include <type_traits>
 #include <utility>
 
 /* "base/*" refers to the base folder inside chromium - https://github.com/chromium/chromium/tree/main/base */
 #include "base/check.h"
 #include "base/check_is_test.h"
 #include "base/containers/span.h"
 #include "base/files/file_util.h"
 #include "base/functional/bind.h"
 #include "base/hash/hash.h"
 #include "base/location.h"
 #include "base/logging.h"
 #include "base/metrics/histogram_functions_internal_overloads.h"
 #include "base/strings/strcat.h"
 #include "base/strings/string_split.h"
 #include "base/task/bind_post_task.h"
 #include "base/timer/elapsed_timer.h"
 
 #include "third_party/tflite_support/src/tensorflow_lite_support/cc/port/statusor.h"
 #include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/text_embedder.h"
 #include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/core/proto/base_options.pb.h"
 #include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/core/proto/external_file.pb.h"
 #include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/proto/text_embedder_options.pb.h"
 #include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/utils/text_op_resolver.h"
 
 using TFLiteTextEmbedder = tflite::task::text::TextEmbedder;
 using TFLiteTextEmbedderOptions = tflite::task::text::TextEmbedderOptions;
 using tflite::task::text::CreateTextOpResolver;
 
 namespace ai_chat {
 
 // static
 std::unique_ptr<TextEmbedder, base::OnTaskRunnerDeleter> TextEmbedder::Create(
     const base::FilePath& model_path,
     scoped_refptr<base::SequencedTaskRunner> embedder_task_runner) {
   if (model_path.empty()) {
     return std::unique_ptr<TextEmbedder, base::OnTaskRunnerDeleter>(
         nullptr, base::OnTaskRunnerDeleter(embedder_task_runner));
   }
   auto embedder = std::unique_ptr<TextEmbedder, base::OnTaskRunnerDeleter>(
       new TextEmbedder(model_path, embedder_task_runner),
       base::OnTaskRunnerDeleter(embedder_task_runner));
   return embedder;
 }
 
 TextEmbedder::TextEmbedder(
     const base::FilePath& model_path,
     scoped_refptr<base::SequencedTaskRunner> embedder_task_runner)
     : model_path_(model_path),
       owner_task_runner_(base::SequencedTaskRunner::GetCurrentDefault()),
       embedder_task_runner_(embedder_task_runner) {
   DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
 }
 
 TextEmbedder::~TextEmbedder() = default;
 
 bool TextEmbedder::IsInitialized() const {
     DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
     base::AutoLock auto_lock(lock_);
     return tflite_text_embedder_ != nullptr;
 }
 
 void TextEmbedder::Initialize(base::OnceCallback<void(bool)> callback) {
     DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
   
     embedder_task_runner_->PostTask(
         FROM_HERE,
         base::BindOnce(&TextEmbedder::InitializeEmbedder,
                        weak_ptr_factory_.GetWeakPtr(),
                        base::BindPostTaskToCurrentDefault(std::move(callback))));
 }
 
 void TextEmbedder::InitializeEmbedder(InitializeCallback callback) {
     DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());
     if (!base::PathExists(model_path_)) {
       std::move(callback).Run(false);
       return;
     }
     TFLiteTextEmbedderOptions options;
     std::string file_content;
     if (!base::ReadFileToString(model_path_, &file_content)) {
       std::move(callback).Run(false);
       return;
     }
     *options.mutable_base_options()
          ->mutable_model_file()
          ->mutable_file_content() = std::move(file_content);
     auto maybe_text_embedder =
         TFLiteTextEmbedder::CreateFromOptions(options, CreateTextOpResolver());
     if (!maybe_text_embedder.ok()) {
       VLOG(1) << maybe_text_embedder.status().ToString();
       std::move(callback).Run(false);
       return;
     }
   
     base::AutoLock auto_lock(lock_);
     tflite_text_embedder_ = std::move(maybe_text_embedder.value());
     std::move(callback).Run(true);
 }
 
 absl::Status TextEmbedder::EmbedText(
     const std::string& text,
     tflite::task::processor::EmbeddingResult& embedding) {
   DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());
   if (!tflite_text_embedder_) {
     return absl::FailedPreconditionError("TextEmbedder is not initialized.");
   }
   auto maybe_embedding = tflite_text_embedder_->Embed(text);
   if (!maybe_embedding.ok()) {
     return maybe_embedding.status();
   }
   embedding = maybe_embedding.value();
   return absl::OkStatus();
 }
 
 // Given an array of strings (tab-title + origin), generate embeddings for all
 absl::Status TextEmbedder::EmbedTabs() {
     DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());
     if (tabs_.empty()) {
       return absl::FailedPreconditionError("No segments to embed.");
     }
     embeddings_.clear();
     for (const auto& tab : tabs_) {
       tflite::task::processor::EmbeddingResult embedding;
       auto status = EmbedText(tab, embedding);
       if (!status.ok()) {
         return status;
       }
       embeddings_.push_back(embedding);
     }
     return absl::OkStatus();
 }
 
 
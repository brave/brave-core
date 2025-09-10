// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/fast_vlm_service.h"

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/browser_context.h"

namespace local_ai {

// PendingInference implementation
FastVLMService::PendingInference::PendingInference() = default;
FastVLMService::PendingInference::~PendingInference() = default;
FastVLMService::PendingInference::PendingInference(PendingInference&& other) =
    default;
FastVLMService::PendingInference& FastVLMService::PendingInference::operator=(
    PendingInference&& other) = default;

namespace {

base::FilePath GetDefaultModelPath() {
  // Default path for FastVLM model
  return base::FilePath("/Users/darkdh/Projects/FastVLM-0.5B-ONNX");
}

}  // namespace

FastVLMService::FastVLMService(content::BrowserContext* context)
    : browser_context_(context),
      model_path_(GetDefaultModelPath()),
      owner_task_runner_(base::SequencedTaskRunner::GetCurrentDefault()),
      background_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_BLOCKING})) {
  // Don't create executor on main thread - will be created on background thread
}

FastVLMService::~FastVLMService() = default;

void FastVLMService::RunInference(
    const std::vector<uint8_t>& image_data,
    const std::string& prompt,
    int max_tokens,
    base::OnceCallback<void(bool success, const std::string& result)>
        callback) {
  LOG(INFO) << "Running FastVLM inference: " << prompt;

  // Post inference to background thread and handle initialization there
  background_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&FastVLMService::RunInferenceOnBackgroundThread,
                     weak_ptr_factory_.GetWeakPtr(), image_data, prompt,
                     max_tokens,
                     base::BindPostTaskToCurrentDefault(std::move(callback))));
}

void FastVLMService::RunInferenceOnBackgroundThread(
    const std::vector<uint8_t>& image_data,
    const std::string& prompt,
    int max_tokens,
    base::OnceCallback<void(bool success, const std::string& result)>
        callback) {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());

  // If model is loaded, run inference immediately
  if (model_loaded_ && executor_) {
    InferenceRequest request;
    request.image_data = image_data;
    request.text_prompt = prompt;
    request.max_tokens = max_tokens;

    executor_->RunInference(
        request,
        base::BindOnce(&FastVLMService::OnInferenceComplete,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  // Queue the inference request for later processing
  PendingInference pending;
  pending.image_data = image_data;
  pending.prompt = prompt;
  pending.max_tokens = max_tokens;
  pending.callback = std::move(callback);
  pending_inferences_.push_back(std::move(pending));

  // Start loading model if not already in progress
  if (!loading_in_progress_ && !executor_) {
    loading_in_progress_ = true;
    executor_ = std::make_unique<FastVLMExecutor>();

    // Initialize direct ONNX Runtime execution
    LOG(INFO) << "Initializing FastVLM executor with direct ONNX Runtime";
    executor_->InitializeOnnxRuntime();

    LOG(INFO) << "Starting FastVLM model loading from: " << model_path_;

    executor_->LoadModel(
        model_path_,
        base::BindOnce(&FastVLMService::OnModelLoadedOnBackgroundThread,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void FastVLMService::OnModelLoadedOnBackgroundThread(bool success) {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());

  loading_in_progress_ = false;
  model_loaded_ = success;

  if (success) {
    LOG(INFO) << "FastVLM model loaded successfully, processing "
              << pending_inferences_.size() << " pending inferences";
    ProcessPendingInferences();
  } else {
    LOG(ERROR) << "Failed to load FastVLM model, failing "
               << pending_inferences_.size() << " pending inferences";
    // Fail all pending inferences
    for (auto& pending : pending_inferences_) {
      std::move(pending.callback).Run(false, "Failed to load model");
    }
    pending_inferences_.clear();
  }
}

void FastVLMService::ProcessPendingInferences() {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(model_loaded_ && executor_);

  for (auto& pending : pending_inferences_) {
    InferenceRequest request;
    request.image_data = pending.image_data;
    request.text_prompt = pending.prompt;
    request.max_tokens = pending.max_tokens;

    executor_->RunInference(request,
                            base::BindOnce(&FastVLMService::OnInferenceComplete,
                                           weak_ptr_factory_.GetWeakPtr(),
                                           std::move(pending.callback)));
  }

  pending_inferences_.clear();
}

void FastVLMService::OnInferenceComplete(
    base::OnceCallback<void(bool success, const std::string& result)> callback,
    InferenceResult result) {
  if (result.success) {
    LOG(INFO) << "FastVLM inference completed successfully";
    std::move(callback).Run(true, result.generated_text);
  } else {
    LOG(ERROR) << "FastVLM inference failed: " << result.error_message;
    std::move(callback).Run(false, result.error_message);
  }
}

bool FastVLMService::IsReady() const {
  return model_loaded_;
}

std::string FastVLMService::GetModelStatus() const {
  if (loading_in_progress_) {
    return "Model loading in progress";
  } else if (!model_loaded_) {
    return "Model not loaded";
  } else {
    return "Ready";
  }
}

}  // namespace local_ai

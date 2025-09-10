// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_LOCAL_AI_FAST_VLM_SERVICE_H_
#define BRAVE_BROWSER_LOCAL_AI_FAST_VLM_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/local_ai/browser/fast_vlm_executor.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace content {
class BrowserContext;
}

namespace local_ai {

// Browser-level service that manages FastVLM execution with GPU process
// integration
class FastVLMService : public KeyedService {
 public:
  explicit FastVLMService(content::BrowserContext* context);
  ~FastVLMService() override;

  FastVLMService(const FastVLMService&) = delete;
  FastVLMService& operator=(const FastVLMService&) = delete;

  // Run vision-language inference
  void RunInference(
      const std::vector<uint8_t>& image_data,
      const std::string& prompt,
      int max_tokens,
      base::OnceCallback<void(bool success, const std::string& result)>
          callback);

  // Check if the service is ready for inference
  bool IsReady() const;

  // Get model status information
  std::string GetModelStatus() const;

 private:
  void RunInferenceOnBackgroundThread(
      const std::vector<uint8_t>& image_data,
      const std::string& prompt,
      int max_tokens,
      base::OnceCallback<void(bool success, const std::string& result)>
          callback);
  void OnModelLoadedOnBackgroundThread(bool success);
  void ProcessPendingInferences();
  void OnInferenceComplete(
      base::OnceCallback<void(bool success, const std::string& result)>
          callback,
      InferenceResult result);

  raw_ptr<content::BrowserContext> browser_context_;
  std::unique_ptr<FastVLMExecutor> executor_;
  base::FilePath model_path_;

  // Task runners for threading
  scoped_refptr<base::SequencedTaskRunner> owner_task_runner_;
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  bool model_loaded_ = false;
  bool loading_in_progress_ = false;

  // Queue for pending inference requests while model is loading
  struct PendingInference {
    PendingInference();
    ~PendingInference();
    PendingInference(PendingInference&& other);
    PendingInference& operator=(PendingInference&& other);

    std::vector<uint8_t> image_data;
    std::string prompt;
    int max_tokens;
    base::OnceCallback<void(bool success, const std::string& result)> callback;
  };
  std::vector<PendingInference> pending_inferences_;

  base::WeakPtrFactory<FastVLMService> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_LOCAL_AI_FAST_VLM_SERVICE_H_

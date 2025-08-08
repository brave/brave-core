/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 #ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEXT_EMBEDDER_H_
 #define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEXT_EMBEDDER_H_
 
 #include <stddef.h>
 #include <cstdint>
 #include <memory>
 #include <string>
 #include <utility>
 #include <vector>
 
 #include "base/files/file_path.h"
 #include "base/functional/callback.h"
 #include "base/memory/scoped_refptr.h"
 #include "base/memory/weak_ptr.h"
 #include "base/synchronization/lock.h"
 #include "base/task/sequenced_task_runner.h"
 #include "base/types/expected.h"
 
 /* During build, the .pb.h is generated from the .proto at
 https://github.com/tensorflow/tflite-support/blob/master/tensorflow_lite_support/cc/task/processor/proto/embedding.proto */
 #include "tensorflow_lite_support/cc/task/processor/proto/embedding.pb.h"
 
 /* from https://source.chromium.org/chromium/chromium/src/+/main:third_party/ */
 #include "third_party/abseil-cpp/absl/status/status.h"
 #include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/text_embedder.h"
 
 namespace tflite {
 namespace task {
 namespace text {
 class TextEmbedder;
 }  // namespace text
 }  // namespace task
 }  // namespace tflite
 
 namespace base {
 class SequencedTaskRunner;
 }  // namespace base
 
 namespace ai_chat {
 // TextEmbedder is a wrapper class for tflite::task::text::TextEmbedder.and
 // runs all the operations on a separate sequence task runner to avoid blocking
 // owner sequence runner ex. brwoser UI thread.
 class TextEmbedder {
     public:
         static std::unique_ptr<TextEmbedder, base::OnTaskRunnerDeleter> Create(
             const base::FilePath& model_path,
             scoped_refptr<base::SequencedTaskRunner> embedder_task_runner);
 
         virtual ~TextEmbedder();
         TextEmbedder(const TextEmbedder&) = delete;
         TextEmbedder& operator=(const TextEmbedder&) = delete;
 
         virtual bool IsInitialized() const;
 
         // Initialize tflite::task::text::TextEmbedder with the model file.
         // Since tflite on Windows doesn't support file path loading so we read the
         // model file and pass it through file content.
         using InitializeCallback = base::OnceCallback<void(bool)>;
         virtual void Initialize(InitializeCallback callback);
 
         
         // DECLARE FUNCTIONS FOR TABS HERE //
 
         // Cancel all the pending tflite tasks on the embedder task runner.
         // Should be used right before the TextEmbedder is destroyed to avoid long
         // running tflite tasks blocking shutdown.
         void CancelAllTasks();
 
     protected:
         explicit TextEmbedder(
             const base::FilePath& model_path,
             scoped_refptr<base::SequencedTaskRunner> embedder_task_runner);
             
     private:
         friend class TextEmbedderUnitTest;
 
         void InitializeEmbedder(base::OnceCallback<void(bool)> callback);
 
         absl::Status EmbedText(const std::string& text,
             tflite::task::processor::EmbeddingResult& embedding);
         
         
         // Declare the functions or variables for 'suggest tabs for group' feature here
         absl::Status EmbedTabs();

         // an array containing strings (tab-title + origin)
         // should we have a limit on the length of this array?
         std::vector<std::string> tabs_; 
 
         std::vector<tflite::task::processor::EmbeddingResult> embeddings_;
 
         // Lock used to ensure mutual exclusive access to |tflite_text_embedder_|
         // when setting unique_ptr and accessing it from |owner_task_runner_|.
         mutable base::Lock lock_;
         std::unique_ptr<tflite::task::text::TextEmbedder> tflite_text_embedder_;
 
         const base::FilePath model_path_;
         scoped_refptr<base::SequencedTaskRunner> owner_task_runner_;
         scoped_refptr<base::SequencedTaskRunner> embedder_task_runner_;
 
         base::WeakPtrFactory<TextEmbedder> weak_ptr_factory_{this};
 };
 }  // namespace ai_chat
 #endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEXT_EMBEDDER_H_
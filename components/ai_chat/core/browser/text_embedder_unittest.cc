/* Copyright (c) 2025 The Brave Authors. All rights reserved.
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/text_embedder.h"

#include <string.h>
#include <memory>
#include <string_view>
#include <utility>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/local_models_updater.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class TextEmbedderUnitTest : public testing::Test {
    public:
        TextEmbedderUnitTest()
            : embedder_(nullptr, base::OnTaskRunnerDeleter(nullptr)) {}

        ~TextEmbedderUnitTest() override = default;

        // prepare test env before each test run involving TextEmbedderUnitTest
        void SetUp() override {
            // task runner where TFLite embedding work will run
            embedder_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
                {base::MayBlock(), base::TaskPriority::BEST_EFFORT});
            
            // fetch path to test data dir.
            base::FilePath test_dir =
                base::PathService::CheckedGet(brave::DIR_TEST_DATA);

            // constructs sub path like ../leo/local-models-updater
            model_dir_ =
                test_dir.AppendASCII("leo").AppendASCII("local-models-updater");

            // create text embedder using the given model file. also passes test runner so it can work off thread.
            // kUniversalQAModelName is defined in local_models_updater.h file. 
            embedder_ = TextEmbedder::Create(
                 // base::FilePath(TEMP_MODEL_PATH),
                base::FilePath(model_dir_.AppendASCII(kUniversalQAModelName)),
                embedder_task_runner_);
            
            // if embedder creation fails, test should fail
            ASSERT_TRUE(embedder_);

            base::RunLoop run_loop;
            embedder_->Initialize(
                base::BindLambdaForTesting([&run_loop](bool initialized) {
                ASSERT_TRUE(initialized);
                run_loop.Quit();
            }));

            run_loop.Run();
            ASSERT_TRUE(embedder_->IsInitialized());
        }

    protected:
        std::unique_ptr<ai_chat::TextEmbedder, base::OnTaskRunnerDeleter> embedder_;
        base::FilePath model_dir_;
        scoped_refptr<base::SequencedTaskRunner> embedder_task_runner_;
        base::test::TaskEnvironment task_environment_;

    };


    TEST_F(TextEmbedderUnitTest, Create) {
        // EXPECT_FALSE(TextEmbedder::Create(base::FilePath(), embedder_task_runner_));
        // Invalid model path is tested in TextEmbedderUnitTest.Initialize.
        // EXPECT_TRUE(TextEmbedder::Create(
        //     // base::FilePath(TEMP_MODEL_PATH),
        //     base::FilePath(model_dir_.AppendASCII("model.tflite")),
        //     embedder_task_runner_));
        EXPECT_TRUE(TextEmbedder::Create(
            base::FilePath(model_dir_.AppendASCII(kUniversalQAModelName)),
            embedder_task_runner_));
      }

} // namespace ai_chat


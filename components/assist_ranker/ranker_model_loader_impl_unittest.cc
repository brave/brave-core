/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/assist_ranker/ranker_model_loader_impl.h"

#include <initializer_list>
#include <memory>
#include <vector>

#include "base/files/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/assist_ranker/proto/ranker_model.pb.h"
#include "components/assist_ranker/proto/translate_ranker_model.pb.h"
#include "components/assist_ranker/ranker_model.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using assist_ranker::RankerModel;
using assist_ranker::RankerModelLoaderImpl;
using assist_ranker::RankerModelStatus;

class RankerModelLoaderImplTest : public ::testing::Test {
 public:
  RankerModelLoaderImplTest(const RankerModelLoaderImplTest&) = delete;
  RankerModelLoaderImplTest& operator=(const RankerModelLoaderImplTest&) =
      delete;

 protected:
  RankerModelLoaderImplTest();

  void SetUp() override;

  // Helper method to drive the loader for |model_path| and |model_url|.
  bool DoLoaderTest(const base::FilePath& model_path, const GURL& model_url);

  // Initialize the "remote" model data used for testing.
  void InitRemoteModels();

  // Helper method used by InitRemoteModels()
  void InitModel(const GURL& model_url,
                 const base::Time& last_modified,
                 const base::TimeDelta& cache_duration,
                 RankerModel* model);

  // Implements RankerModelLoaderImpl's ValidateModelCallback interface.
  RankerModelStatus ValidateModel(const RankerModel& model);

  // Implements RankerModelLoaderImpl's OnModelAvailableCallback interface.
  void OnModelAvailable(std::unique_ptr<RankerModel> model);

  // Sets up the task scheduling/task-runner environment for each test.
  base::test::TaskEnvironment task_environment_;

  // Override the default URL loader to return custom responses for tests.
  network::TestURLLoaderFactory test_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;

  // Model URLS.
  GURL remote_model_url_;

  // Model Data.
  RankerModel remote_model_;
};

RankerModelLoaderImplTest::RankerModelLoaderImplTest() {
  test_shared_loader_factory_ =
      base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
          &test_loader_factory_);
}

void RankerModelLoaderImplTest::SetUp() {
  // Setup the model URLs.
  remote_model_url_ = GURL("https://some.url.net/good.model.bin");

  // Initialize the model data.
  ASSERT_NO_FATAL_FAILURE(InitRemoteModels());
}

// static
bool RankerModelLoaderImplTest::DoLoaderTest(const base::FilePath& model_path,
                                             const GURL& model_url) {
  auto loader = std::make_unique<RankerModelLoaderImpl>(
      base::BindRepeating(&RankerModelLoaderImplTest::ValidateModel,
                          base::Unretained(this)),
      base::BindRepeating(&RankerModelLoaderImplTest::OnModelAvailable,
                          base::Unretained(this)),
      test_shared_loader_factory_, model_path, model_url,
      "RankerModelLoaderImplTest");
  loader->NotifyOfRankerActivity();
  task_environment_.RunUntilIdle();

  return true;
}

void RankerModelLoaderImplTest::InitRemoteModels() {
  InitModel(remote_model_url_, base::Time(), base::TimeDelta(), &remote_model_);
}

void RankerModelLoaderImplTest::InitModel(const GURL& model_url,
                                          const base::Time& last_modified,
                                          const base::TimeDelta& cache_duration,
                                          RankerModel* model) {
  ASSERT_TRUE(model != nullptr);
  model->mutable_proto()->Clear();

  auto* metadata = model->mutable_proto()->mutable_metadata();
  if (!model_url.is_empty())
    metadata->set_source(model_url.spec());
  if (!last_modified.is_null()) {
    auto last_modified_sec = (last_modified - base::Time()).InSeconds();
    metadata->set_last_modified_sec(last_modified_sec);
  }
  if (!cache_duration.is_zero())
    metadata->set_cache_duration_sec(cache_duration.InSeconds());

  auto* translate = model->mutable_proto()->mutable_translate();
  translate->set_version(1);

  auto* logit = translate->mutable_translate_logistic_regression_model();
  logit->set_bias(0.1f);
  logit->set_accept_ratio_weight(0.2f);
  logit->set_decline_ratio_weight(0.3f);
  logit->set_ignore_ratio_weight(0.4f);
}

RankerModelStatus RankerModelLoaderImplTest::ValidateModel(
    const RankerModel& model) {
  NOTREACHED();
  return RankerModelStatus::OK;
}

void RankerModelLoaderImplTest::OnModelAvailable(
    std::unique_ptr<RankerModel> model) {
  NOTREACHED();
}

}  // namespace

TEST_F(RankerModelLoaderImplTest, LoadRemoteRankerNoFetch) {
  bool network_access_occurred = false;
  test_loader_factory_.SetInterceptor(
    base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
                                   network_access_occurred = true;
                               }));
  ASSERT_TRUE(DoLoaderTest(base::FilePath(), remote_model_url_));
  EXPECT_FALSE(network_access_occurred);
}

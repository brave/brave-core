
// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/background_history_querier.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/task/bind_post_task.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/url_row.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace brave_news {

namespace {
class MockHistoryService : public history::HistoryService {
 public:
  MockHistoryService() = default;
  MOCK_METHOD4(QueryHistory,
               base::CancelableTaskTracker::TaskId(
                   const std::u16string& text_query,
                   const history::QueryOptions& options,
                   QueryHistoryCallback callback,
                   base::CancelableTaskTracker* tracker));
};
}  // namespace

using testing::_;

class BraveNewsBackgroundHistoryQuerierTest : public testing::Test {
 public:
  BraveNewsBackgroundHistoryQuerierTest()
      : bg_task_runner_(base::ThreadPool::CreateSingleThreadTaskRunner(
            {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {
    querier_ = MakeHistoryQuerier(
        history_service_.AsWeakPtr(), base::BindLambdaForTesting([this]() {
          return fake_destroyed_ ? nullptr : &tracker_;
        }));
  }
  BraveNewsBackgroundHistoryQuerierTest(
      const BraveNewsBackgroundHistoryQuerierTest&) = delete;
  BraveNewsBackgroundHistoryQuerierTest& operator=(
      const BraveNewsBackgroundHistoryQuerierTest&) = delete;
  ~BraveNewsBackgroundHistoryQuerierTest() override = default;

  void QueryOnBgRunner() {
    auto on_result = base::BindPostTaskToCurrentDefault(base::BindOnce(
        [](history::QueryResults* results, base::OnceClosure quit,
           history::QueryResults query_result) {
          *results = std::move(query_result);
          std::move(quit).Run();
        },
        &results_, loop_.QuitClosure()));

    bg_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](BackgroundHistoryQuerier querier,
               base::OnceCallback<void(history::QueryResults)> callback) {
              querier.Run(std::move(callback));
            },
            querier_, std::move(on_result)));
  }

  const history::QueryResults& Wait() {
    loop_.Run();
    return results_;
  }

  void FakeDestroyed() { fake_destroyed_ = true; }

 protected:
  MockHistoryService history_service_;

 private:
  content::BrowserTaskEnvironment task_environment_;

  base::CancelableTaskTracker tracker_;
  scoped_refptr<base::SequencedTaskRunner> bg_task_runner_;
  BackgroundHistoryQuerier querier_;

  history::QueryResults results_;
  base::RunLoop loop_;
  bool fake_destroyed_ = false;
};

TEST_F(BraveNewsBackgroundHistoryQuerierTest, CanGetHistoryOffMainThread) {
  EXPECT_CALL(history_service_, QueryHistory(_, _, _, _))
      .WillRepeatedly(testing::WithArg<2>([&](QueryHistoryCallback callback) {
        history::QueryResults results;
        auto result =
            history::URLResult(GURL("https://example.com"), base::Time::Now());
        results.SetURLResults({result});
        std::move(callback).Run(std::move(results));
        return 0;
      }));

  QueryOnBgRunner();
  auto& result = Wait();

  EXPECT_EQ(1u, result.size());
  EXPECT_EQ(GURL("https://example.com"), result.back().url());
}

TEST_F(BraveNewsBackgroundHistoryQuerierTest,
       BackgroundHistoryNotRequestedWhenDestroyed) {
  EXPECT_CALL(history_service_, QueryHistory(_, _, _, _))
      .WillRepeatedly(testing::WithArg<2>([&](QueryHistoryCallback callback) {
        history::QueryResults results;
        auto result =
            history::URLResult(GURL("https://example.com"), base::Time::Now());
        results.SetURLResults({result});
        std::move(callback).Run(std::move(results));
        return 0;
      }));

  QueryOnBgRunner();
  FakeDestroyed();
  auto& result = Wait();
  EXPECT_TRUE(result.empty());
}

}  // namespace brave_news

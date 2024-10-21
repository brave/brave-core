/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/publisher.h"

#include <iostream>
#include <utility>

#include "base/containers/flat_map.h"
#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal::publisher {

class RewardsPublisherTest : public RewardsEngineTest {
 protected:
  void CreatePublisherInfoList(std::vector<mojom::PublisherInfoPtr>* list) {
    double prev_score;
    for (int ix = 0; ix < 50; ix++) {
      mojom::PublisherInfoPtr info = mojom::PublisherInfo::New();
      info->id = "example" + std::to_string(ix) + ".com";
      info->duration = 50;
      if (ix == 0) {
        info->score = 24;
      } else {
        info->score = prev_score / 2;
      }
      prev_score = info->score;
      info->reconcile_stamp = 0;
      info->visits = 5;
      list->push_back(std::move(info));
    }
  }

  double GetA() { return engine().Get<Prefs>().GetDouble(prefs::kScoreA); }

  double GetB() { return engine().Get<Prefs>().GetDouble(prefs::kScoreB); }

  Publisher publisher_{engine()};
};

TEST_F(RewardsPublisherTest, CalcScoreConsts5) {
  publisher_.CalcScoreConsts(5);

  ASSERT_EQ(GetA(), 14500);
  ASSERT_EQ(GetB(), -14000);
}

TEST_F(RewardsPublisherTest, CalcScoreConsts8) {
  publisher_.CalcScoreConsts(8);

  ASSERT_EQ(GetA(), 14200);
  ASSERT_EQ(GetB(), -13400);
}

TEST_F(RewardsPublisherTest, CalcScoreConsts60) {
  publisher_.CalcScoreConsts(60);

  ASSERT_EQ(GetA(), 9000);
  ASSERT_EQ(GetB(), -3000);
}

TEST_F(RewardsPublisherTest, concaveScore) {
  publisher_.CalcScoreConsts(5);
  EXPECT_NEAR(publisher_.concaveScore(5), 1, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(15), 1.06285, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(60), 1.28703, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(1000), 3.15289, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(10000), 8.80133, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(150000), 32.6498, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(500000), 59.2068, 0.001f);

  publisher_.CalcScoreConsts(8);
  EXPECT_NEAR(publisher_.concaveScore(5), 0.979606, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(15), 1.04477, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(60), 1.27505, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(1000), 3.16717, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(10000), 8.8769, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(150000), 32.9766, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(500000), 59.8128, 0.001f);

  publisher_.CalcScoreConsts(60);
  EXPECT_NEAR(publisher_.concaveScore(5), 0.455342, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(15), 0.607625, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(60), 1, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(1000), 3.50416, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(10000), 10.7089, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(150000), 40.9918, 0.001f);
  EXPECT_NEAR(publisher_.concaveScore(500000), 74.7025, 0.001f);
}

TEST_F(RewardsPublisherTest, synopsisNormalizerInternal) {
  // create test PublisherInfo list
  std::vector<mojom::PublisherInfoPtr> new_list;
  std::vector<mojom::PublisherInfoPtr> list;
  CreatePublisherInfoList(&list);
  publisher_.synopsisNormalizerInternal(&new_list, &list, 0);

  // simulate exclude and re-normalize
  new_list.erase(new_list.begin() + 3);
  std::vector<mojom::PublisherInfoPtr> new_list2;
  publisher_.synopsisNormalizerInternal(&new_list2, &new_list, 0);
  new_list2.erase(new_list2.begin() + 4);
  std::vector<mojom::PublisherInfoPtr> new_list3;
  publisher_.synopsisNormalizerInternal(&new_list3, &new_list2, 0);
  new_list3.erase(new_list3.begin() + 5);
  std::vector<mojom::PublisherInfoPtr> new_list4;
  publisher_.synopsisNormalizerInternal(&new_list4, &new_list3, 0);
  new_list4.erase(new_list4.begin() + 6);
  std::vector<mojom::PublisherInfoPtr> new_list5;
  publisher_.synopsisNormalizerInternal(&new_list5, &new_list4, 0);

  for (const auto& element : new_list5) {
    ASSERT_GE((int32_t)element->percent, 0);
    ASSERT_LE((int32_t)element->percent, 100);
  }
}

TEST_F(RewardsPublisherTest, GetShareURL) {
  base::flat_map<std::string, std::string> args;

  // Ensure that missing args results in no output
  EXPECT_EQ(Publisher::GetShareURL(args), "");

  // Ensure that intent looks correct when no tweet ID is specified
  args.insert({"name", "brave"});
  args.insert({"comment", "I just tipped someone using the Brave browser."});
  args.insert({"hashtag", "TipWithBrave"});
  EXPECT_EQ(Publisher::GetShareURL(args),
            "https://twitter.com/intent/tweet?text=I just tipped someone using "
            "the Brave browser.%20%23TipWithBrave");

  // Ensure that intent includes quoted tweet when tweet ID is
  // specified
  args.insert({"tweet_id", "794221010484502528"});
  EXPECT_EQ(Publisher::GetShareURL(args),
            "https://twitter.com/intent/tweet?text=I just tipped someone using "
            "the Brave browser.%20%23TipWithBrave"
            "&url=https://twitter.com/brave/status/794221010484502528");
}

}  // namespace brave_rewards::internal::publisher

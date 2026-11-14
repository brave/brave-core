// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/history_embeddings_service_factory.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_history_embeddings {

// Brave builds its own HistoryEmbeddingsService. Upstream's must never be
// built alongside it: both would index the same visits into the same
// HistoryEmbeddings database, and upstream's tab helper would attach to every
// tab.
class HistoryEmbeddingsServiceFactoryBrowserTest : public InProcessBrowserTest {
 public:
  HistoryEmbeddingsServiceFactoryBrowserTest() {
    feature_list_.InitAndEnableFeature(kHistoryEmbeddings);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Leaves the setting on for the run below, which captures it at startup.
IN_PROC_BROWSER_TEST_F(HistoryEmbeddingsServiceFactoryBrowserTest,
                       PRE_OnlyBraveServiceIsBuilt) {
  browser()->GetProfile()->GetPrefs()->SetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled, true);
}

IN_PROC_BROWSER_TEST_F(HistoryEmbeddingsServiceFactoryBrowserTest,
                       OnlyBraveServiceIsBuilt) {
  Profile* profile = browser()->GetProfile();
  ASSERT_TRUE(profile->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled));

  EXPECT_TRUE(HistoryEmbeddingsServiceFactory::GetForProfile(profile));
  EXPECT_FALSE(::HistoryEmbeddingsServiceFactory::GetForProfile(profile));
}

}  // namespace brave_history_embeddings

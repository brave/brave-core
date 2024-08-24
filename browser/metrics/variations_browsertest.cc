/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/test/metrics/histogram_tester.h"
#include "chrome/browser/browser_process.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/variations/service/variations_field_trial_creator_base.h"
#include "components/variations/variations_test_utils.h"
#include "content/public/test/browser_test.h"

namespace variations {
namespace {

BASE_FEATURE(kVariationsTestFeature,
             "VariationsTestFeature",
             base::FEATURE_DISABLED_BY_DEFAULT);

// The seed is signed with the private key of the Brave variations server.
const SignedSeedData& GetBraveSignedSeedData() {
  static const char* study_names[] = {"VariationsTestStudy"};

  constexpr char kBase64UncompressedData[] =
      "CiA5NDIyMDlmNWEwYzRkOTFiYThiZDk4N2ZlOGU5NTcxMBJMChNWYXJpYXRpb25zVGVzdFN0"
      "dWR5OAFKJAoHRW5hYmxlZBBkYhcKFVZhcmlhdGlvbnNUZXN0RmVhdHVyZUoLCgdEZWZhdWx0"
      "EABgASIaQnJhdmUgdmFyaWF0aW9ucyB0ZXN0IHNlZWQ=";

  constexpr char kBase64CompressedData[] =
      "H4sIAAAAAAAAA+"
      "NSsDQxMjKwTDNNNEg2SbE0TEq0SEqxtDBPS7VItTQ1NzQQ8uESDkssykwsyczPKw5JLS4JLi"
      "lNqbRg9FLhYnfNS0zKSU0RSEkS5xJFVeWWmlhSWpTqxc3F7pKalliaUyLAkMCoJOVUlFiWql"
      "AGV6tQAlSsUJyamgIARXSxvIwAAAA=";

  constexpr char kBase64Signature[] =
      "MEUCIQDfayOr/"
      "xmQaBThr1i8ARQ1rKEinHluXeR7ve5fqy7L4AIgNym2PbtlL+9142+"
      "T8gUjjEsoT28J3HqE4IEa1eFvKLw=";

  static const SignedSeedData kBraveTestSeedData{
      study_names,
      kBase64UncompressedData,
      kBase64CompressedData,
      kBase64Signature,
  };
  return kBraveTestSeedData;
}

}  // namespace

class VariationsBrowserTest : public PlatformBrowserTest {
 public:
  VariationsBrowserTest() { DisableTestingConfig(); }
  ~VariationsBrowserTest() override = default;

 protected:
  base::HistogramTester histogram_tester_;
};

IN_PROC_BROWSER_TEST_F(VariationsBrowserTest, PRE_BraveSeedApplied) {
  PrefService* local_state = g_browser_process->local_state();
  WriteSeedData(local_state, GetBraveSignedSeedData(), kRegularSeedPrefKeys);

  EXPECT_FALSE(base::FeatureList::IsEnabled(kVariationsTestFeature));
  EXPECT_EQ(variations::GetSeedVersion(), "");
}

// Ideally this test should be enabled on Android, but the Android test
// infrastructure does not run PRE_ tests reliably yet (user data dir is not
// shared). This should be reevaluated when
// https://issues.chromium.org/issues/40200835 is completed.
#if BUILDFLAG(IS_ANDROID)
#define MAYBE_BraveSeedApplied DISABLED_BraveSeedApplied
#else
#define MAYBE_BraveSeedApplied BraveSeedApplied
#endif
IN_PROC_BROWSER_TEST_F(VariationsBrowserTest, MAYBE_BraveSeedApplied) {
  histogram_tester_.ExpectUniqueSample("Variations.SeedUsage",
                                       SeedUsage::kRegularSeedUsed, 1);

  EXPECT_TRUE(FieldTrialListHasAllStudiesFrom(GetBraveSignedSeedData()));

  EXPECT_TRUE(base::FeatureList::IsEnabled(kVariationsTestFeature));
  EXPECT_EQ(variations::GetSeedVersion(), "Brave variations test seed");
}

}  // namespace variations

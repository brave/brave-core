/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/origin_state.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

namespace webcat {

TEST(OriginStateDataTest, InitialStateIsUnverified) {
  auto origin = url::Origin::Create(GURL("https://app.eth"));
  OriginStateData state(origin, OriginState::kUnverified);
  EXPECT_EQ(state.state(), OriginState::kUnverified);
  EXPECT_EQ(state.error(), WebcatError::kNone);
}

TEST(OriginStateDataTest, SetBundleTransitionsToFetched) {
  auto origin = url::Origin::Create(GURL("https://app.eth"));
  OriginStateData state(origin, OriginState::kUnverified);

  Bundle bundle;
  bundle.manifest.app = "https://app.eth";
  bundle.manifest.version = "1.0.0";
  bundle.manifest.default_csp = "default-src 'self'";
  bundle.manifest.default_index = "/index.html";
  bundle.manifest.default_fallback = "/index.html";
  bundle.manifest.files["/index.html"] = "hash1";

  state.SetBundle(bundle);
  EXPECT_EQ(state.state(), OriginState::kBundleFetched);
  ASSERT_TRUE(state.bundle().has_value());
  EXPECT_EQ(state.bundle()->manifest.app, "https://app.eth");
}

TEST(OriginStateDataTest, SetVerifiedTransitionsToVerified) {
  auto origin = url::Origin::Create(GURL("https://app.eth"));
  OriginStateData state(origin, OriginState::kUnverified);

  Bundle bundle;
  bundle.manifest.app = "https://app.eth";
  state.SetBundle(bundle);
  state.SetVerified();
  EXPECT_EQ(state.state(), OriginState::kVerified);
}

TEST(OriginStateDataTest, SetFailedTransitionsToFailed) {
  auto origin = url::Origin::Create(GURL("https://app.eth"));
  OriginStateData state(origin, OriginState::kUnverified);

  state.SetFailed(WebcatError::kCidIntegrityFailed, "CID mismatch");
  EXPECT_EQ(state.state(), OriginState::kFailed);
  EXPECT_EQ(state.error(), WebcatError::kCidIntegrityFailed);
  EXPECT_EQ(state.error_detail(), "CID mismatch");
}

TEST(OriginStateDataTest, SetCidStoresCid) {
  auto origin = url::Origin::Create(GURL("https://app.eth"));
  OriginStateData state(origin, OriginState::kUnverified);

  state.SetCid("bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqabf3oclgtqy55fbzdi");
  EXPECT_EQ(state.cid(),
            "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqabf3oclgtqy55fbzdi");
}

TEST(OriginStateDataTest, ClearErrorClearsErrorButNotState) {
  auto origin = url::Origin::Create(GURL("https://app.eth"));
  OriginStateData state(origin, OriginState::kUnverified);

  state.SetFailed(WebcatError::kCidIntegrityFailed, "error");
  EXPECT_EQ(state.state(), OriginState::kFailed);

  state.ClearError();
  EXPECT_EQ(state.error(), WebcatError::kNone);
  EXPECT_TRUE(state.error_detail().empty());
}

}  // namespace webcat
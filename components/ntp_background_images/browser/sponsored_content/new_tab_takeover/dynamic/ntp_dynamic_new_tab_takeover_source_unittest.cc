/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cstddef>

#include "brave/components/ntp_background_images/browser/sponsored_content/new_tab_takeover/dynamic/test/ntp_dynamic_new_tab_takeover_source_test_base.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/test/ntp_sponsored_content_source_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

base::FilePath GetComponentPath() {
  return test::GetSponsoredImagesComponentPath()
      .AppendASCII("new_tab_takeover")
      .AppendASCII("dynamic");
}

}  // namespace

class NTPDynamicNewTabTakeoverSourceTest
    : public test::NTPDynamicNewTabTakeoverSourceTestBase {
 protected:
  void SetUp() override {
    test::NTPDynamicNewTabTakeoverSourceTestBase::SetUp();
    SimulateDeprecatedOnSponsoredContentDidUpdate(GetComponentPath());
  }
};

TEST_F(NTPDynamicNewTabTakeoverSourceTest, StartDataRequest) {
  EXPECT_THAT(
      test::StartDataRequest(
          url_data_source(),
          GURL(
              R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/index.html)")),
      ::testing::Not(::testing::IsEmpty()));
}

TEST_F(NTPDynamicNewTabTakeoverSourceTest,
       DoNotStartDataRequestIfContentIsReferencingParentDirectory) {
  EXPECT_THAT(test::StartDataRequest(
                  url_data_source(),
                  GURL("chrome-untrusted://new-tab-takeover/restricted.jpg")),
              ::testing::IsEmpty());
  EXPECT_THAT(
      test::StartDataRequest(
          url_data_source(),
          GURL(
              R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/../restricted.jpg)")),
      ::testing::IsEmpty());
}

TEST_F(NTPDynamicNewTabTakeoverSourceTest,
       DoNotStartDataRequestIfContentIsFromAnotherCampaign) {
  EXPECT_THAT(
      test::StartDataRequest(
          url_data_source(),
          GURL(
              R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/../3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/index.html)")),
      ::testing::IsEmpty());
}

TEST_F(NTPDynamicNewTabTakeoverSourceTest,
       DoNotStartDataRequestIfContentIsOutsideOfSandbox) {
  EXPECT_THAT(test::StartDataRequest(
                  url_data_source(),
                  GURL("chrome-untrusted://new-tab-takeover/restricted.jpg")),
              ::testing::IsEmpty());
  EXPECT_THAT(
      test::StartDataRequest(
          url_data_source(),
          GURL(
              R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/../3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/index.html)")),
      ::testing::IsEmpty());
  EXPECT_THAT(
      test::StartDataRequest(
          url_data_source(),
          GURL(
              R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/../restricted.jpg)")),
      ::testing::IsEmpty());
}

// `DUMP_WILL_BE_NOTREACHED()` aborts the process in non-official DCHECK builds.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
TEST_F(NTPDynamicNewTabTakeoverSourceTest,
       DoNotStartDataRequestIfContentDoesNotExist) {
  EXPECT_THAT(
      test::StartDataRequest(
          url_data_source(),
          GURL(
              R"(chrome-untrusted://new-tab-takeover/non-existent-creative/index.html)")),
      ::testing::IsEmpty());
  EXPECT_THAT(
      test::StartDataRequest(
          url_data_source(),
          GURL(
              R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/non-existent.html)")),
      ::testing::IsEmpty());
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

TEST_F(NTPDynamicNewTabTakeoverSourceTest, GetMimeType) {
  EXPECT_EQ(
      "text/html",
      url_data_source()->GetMimeType(GURL(
          R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/index.html)")));
}

TEST_F(NTPDynamicNewTabTakeoverSourceTest, AllowCaching) {
  EXPECT_FALSE(url_data_source()->AllowCaching());
}

TEST_F(NTPDynamicNewTabTakeoverSourceTest, GetContentSecurityPolicy) {
  for (size_t i = 0;
       i < static_cast<size_t>(network::mojom::CSPDirectiveName::kMaxValue);
       ++i) {
    const auto directive = static_cast<network::mojom::CSPDirectiveName>(i);
    switch (directive) {
      case network::mojom::CSPDirectiveName::FrameAncestors: {
        EXPECT_EQ(
            "frame-ancestors chrome://newtab/ chrome://new-tab-takeover/;",
            url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::Sandbox: {
        EXPECT_EQ("sandbox allow-scripts;",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::DefaultSrc: {
        EXPECT_EQ("default-src 'none';",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::BaseURI: {
        EXPECT_EQ("base-uri 'none';",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::FormAction: {
        EXPECT_EQ("form-action 'none';",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::ScriptSrc: {
        EXPECT_EQ("script-src 'self';",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::StyleSrc: {
        EXPECT_EQ("style-src 'self';",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::FontSrc: {
        EXPECT_EQ("font-src 'self';",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::ImgSrc: {
        EXPECT_EQ("img-src 'self';",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::MediaSrc: {
        EXPECT_EQ("media-src 'self';",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::RequireTrustedTypesFor: {
        EXPECT_EQ("require-trusted-types-for 'script';",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      case network::mojom::CSPDirectiveName::TrustedTypes: {
        EXPECT_EQ("trusted-types;",
                  url_data_source()->GetContentSecurityPolicy(directive));
        break;
      }

      default: {
        EXPECT_THAT(url_data_source()->GetContentSecurityPolicy(directive),
                    ::testing::IsEmpty());
        break;
      }
    }
  }
}

}  // namespace ntp_background_images

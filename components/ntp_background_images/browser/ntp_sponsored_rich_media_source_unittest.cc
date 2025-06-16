/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_source.h"

#include <memory>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service_waiter.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_source_test_util.h"
#include "brave/components/ntp_background_images/browser/switches.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

namespace {

base::FilePath GetComponentPath() {
  return test::GetSponsoredImagesComponentPath().AppendASCII("rich_media");
}

}  // namespace

class NTPSponsoredRichMediaSourceTest : public testing::Test {
 protected:
  void SetUp() override {
    // Disable the super referral wallpaper feature, as it relies on referral
    // codes that are not initialized. Note: Super Referrals are being
    // deprecated, see https://github.com/brave/brave-browser/issues/44403.
    feature_list_.InitAndDisableFeature(
        features::kBraveNTPSuperReferralWallpaper);

    NTPBackgroundImagesService::RegisterLocalStatePrefs(
        pref_service_.registry());

    background_images_service_ = std::make_unique<NTPBackgroundImagesService>(
        /*variations_service*/ nullptr, /*component_update_service=*/nullptr,
        &pref_service_);
    url_data_source_ = std::make_unique<NTPSponsoredRichMediaSource>(
        background_images_service_.get());

    SimulateOnSponsoredImagesDataDidUpdate();
  }

  NTPSponsoredRichMediaSource* url_data_source() {
    return url_data_source_.get();
  }

  std::string StartDataRequest(const GURL& url) {
    CHECK(url_data_source_);

    std::string data;
    content::WebContents::Getter wc_getter;

    base::RunLoop run_loop;
    url_data_source_->StartDataRequest(
        url, wc_getter,
        base::BindOnce(
            [](std::string* data, base::OnceClosure quit_closure,
               scoped_refptr<base::RefCountedMemory> bytes) {
              if (bytes) {
                *data = base::as_string_view(*bytes);
              }
              std::move(quit_closure).Run();
            },
            base::Unretained(&data), run_loop.QuitClosure()));
    run_loop.Run();

    return data;
  }

 private:
  void SimulateOnSponsoredImagesDataDidUpdate() {
    base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
        switches::kOverrideSponsoredImagesComponentPath, GetComponentPath());

    NTPBackgroundImagesServiceWaiter waiter(*background_images_service_);
    background_images_service_->Init();
    waiter.WaitForOnSponsoredImagesDataDidUpdate();
  }

  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple pref_service_;

  std::unique_ptr<NTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<NTPSponsoredRichMediaSource> url_data_source_;
};

TEST_F(NTPSponsoredRichMediaSourceTest, StartDataRequest) {
  EXPECT_THAT(
      StartDataRequest(GURL(
          R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/index.html)")),
      ::testing::Not(::testing::IsEmpty()));
}

TEST_F(NTPSponsoredRichMediaSourceTest,
       DoNotStartDataRequestIfContentIsReferencingParentDirectory) {
  EXPECT_THAT(StartDataRequest(
                  GURL("chrome-untrusted://new-tab-takeover/restricted.jpg")),
              ::testing::IsEmpty());
  EXPECT_THAT(
      StartDataRequest(GURL(
          R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/../restricted.jpg)")),
      ::testing::IsEmpty());
}

TEST_F(NTPSponsoredRichMediaSourceTest,
       DoNotStartDataRequestIfContentIsFromAnotherCampaign) {
  EXPECT_THAT(
      StartDataRequest(GURL(
          R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/../3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/index.html)")),
      ::testing::IsEmpty());
}

TEST_F(NTPSponsoredRichMediaSourceTest,
       DoNotStartDataRequestIfContentIsOutsideOfSandbox3) {
  EXPECT_THAT(StartDataRequest(
                  GURL("chrome-untrusted://new-tab-takeover/restricted.jpg")),
              ::testing::IsEmpty());
  EXPECT_THAT(
      StartDataRequest(GURL(
          R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/../3b36d1b7-5c9b-4625-9227-7c8e9fe6e0b4/index.html)")),
      ::testing::IsEmpty());
  EXPECT_THAT(
      StartDataRequest(GURL(
          R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/../restricted.jpg)")),
      ::testing::IsEmpty());
}

TEST_F(NTPSponsoredRichMediaSourceTest,
       DoNotStartDataRequestIfContentDoesNotExist) {
  EXPECT_THAT(
      StartDataRequest(GURL(
          R"(chrome-untrusted://new-tab-takeover/non-existent-creative/index.html)")),
      ::testing::IsEmpty());
  EXPECT_THAT(
      StartDataRequest(GURL(
          R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/non-existent.html)")),
      ::testing::IsEmpty());
}

TEST_F(NTPSponsoredRichMediaSourceTest, GetMimeType) {
  EXPECT_EQ(
      "text/html",
      url_data_source()->GetMimeType(GURL(
          R"(chrome-untrusted://new-tab-takeover/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/index.html)")));
}

TEST_F(NTPSponsoredRichMediaSourceTest, AllowCaching) {
  EXPECT_FALSE(url_data_source()->AllowCaching());
}

TEST_F(NTPSponsoredRichMediaSourceTest, GetContentSecurityPolicy) {
  for (int i = 0;
       i < static_cast<int>(network::mojom::CSPDirectiveName::kMaxValue); ++i) {
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

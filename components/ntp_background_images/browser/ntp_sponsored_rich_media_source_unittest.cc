/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_source.h"

#include <memory>
#include <string>
#include <utility>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service_waiter.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class NTPSponsoredRichMediaSourceTest : public testing::Test {
 protected:
  void SetUp() override {
    feature_list_.InitAndDisableFeature(
        features::kBraveNTPSuperReferralWallpaper);

    SetUpUrlDataSource();

    SetupSponsoredComponent();
  }

  NTPSponsoredRichMediaSource* url_data_source() {
    return url_data_source_.get();
  }

  std::string StartDataRequest(const GURL& url) {
    std::string data;
    content::WebContents::Getter wc_getter;

    base::RunLoop run_loop;
    url_data_source()->StartDataRequest(
        url, std::move(wc_getter),
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
  void SetUpUrlDataSource() {
    NTPBackgroundImagesService::RegisterLocalStatePrefs(local_pref_.registry());
    background_images_service_ =
        std::make_unique<NTPBackgroundImagesService>(nullptr, &local_pref_);

    url_data_source_ = std::make_unique<NTPSponsoredRichMediaSource>(
        background_images_service_.get());
  }

  void SetupSponsoredComponent() {
    const base::FilePath test_data_file_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);

    const base::FilePath component_file_path =
        test_data_file_path.AppendASCII("ntp_background_images")
            .AppendASCII("components")
            .AppendASCII("rich_media");

    NTPBackgroundImagesServiceWaiter waiter(*background_images_service_);
    background_images_service_->OnSponsoredComponentReady(
        /*is_super_referral=*/false, component_file_path);
    waiter.WaitForOnSponsoredImagesUpdated();
  }

  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_pref_;
  std::unique_ptr<NTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<NTPSponsoredRichMediaSource> url_data_source_;
};

TEST_F(NTPSponsoredRichMediaSourceTest, StartDataRequest) {
  const std::string data = StartDataRequest(
      GURL("chrome-untrusted://rich-media/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/"
           "index.html"));
  EXPECT_THAT(data, ::testing::Not(::testing::IsEmpty()));
}

TEST_F(NTPSponsoredRichMediaSourceTest,
       DoNotStartDataRequestIfReferencingParentDirectory) {
  std::string data =
      StartDataRequest(GURL("chrome-untrusted://rich-media/campaigns.json"));
  EXPECT_THAT(data, ::testing::IsEmpty());

  data = StartDataRequest(GURL(
      "chrome-untrusted://rich-media/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/../"
      "campaigns.json"));
  EXPECT_THAT(data, ::testing::IsEmpty());
}

TEST_F(NTPSponsoredRichMediaSourceTest,
       DoNotStartDataRequestIfContentDoesNotExist) {
  std::string data = StartDataRequest(
      GURL("chrome-untrusted://rich-media/non-existent-creative/index.html"));
  EXPECT_THAT(data, ::testing::IsEmpty());

  data = StartDataRequest(
      GURL("chrome-untrusted://rich-media/aa0b561e-9eed-4aaa-8999-5627bc6b14fd/"
           "non-existent-file.html"));
  EXPECT_THAT(data, ::testing::IsEmpty());
}

TEST_F(NTPSponsoredRichMediaSourceTest, GetMimeType) {
  EXPECT_EQ(url_data_source()->GetMimeType(
                GURL("chrome-untrusted://rich-media/"
                     "aa0b561e-9eed-4aaa-8999-5627bc6b14fd/index.html")),
            "text/html");
}

TEST_F(NTPSponsoredRichMediaSourceTest, AllowCaching) {
  EXPECT_FALSE(url_data_source()->AllowCaching());
}

TEST_F(NTPSponsoredRichMediaSourceTest, GetContentSecurityPolicy) {
  for (int i = 0;
       i < static_cast<int>(network::mojom::CSPDirectiveName::kMaxValue); ++i) {
    const auto directive = static_cast<network::mojom::CSPDirectiveName>(i);
    switch (directive) {
      case network::mojom::CSPDirectiveName::FrameAncestors:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "frame-ancestors chrome://newtab/;");
        break;
      case network::mojom::CSPDirectiveName::Sandbox:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "sandbox allow-scripts;");
        break;
      case network::mojom::CSPDirectiveName::DefaultSrc:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "default-src 'none';");
        break;
      case network::mojom::CSPDirectiveName::BaseURI:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "base-uri 'none';");
        break;
      case network::mojom::CSPDirectiveName::FormAction:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "form-action 'none';");
        break;
      case network::mojom::CSPDirectiveName::ScriptSrc:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "script-src 'self';");
        break;
      case network::mojom::CSPDirectiveName::StyleSrc:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "style-src 'self';");
        break;
      case network::mojom::CSPDirectiveName::ImgSrc:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "img-src 'self';");
        break;
      case network::mojom::CSPDirectiveName::MediaSrc:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "media-src 'self';");
        break;
      case network::mojom::CSPDirectiveName::RequireTrustedTypesFor:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "require-trusted-types-for 'script';");
        break;
      case network::mojom::CSPDirectiveName::TrustedTypes:
        EXPECT_EQ(url_data_source()->GetContentSecurityPolicy(directive),
                  "trusted-types;");
        break;
      default:
        EXPECT_TRUE(
            url_data_source()->GetContentSecurityPolicy(directive).empty());
        break;
    }
  }
}

}  // namespace ntp_background_images

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/car_content_requester.h"
#include <cstdint>
#include <memory>

#include "base/containers/contains.h"
#include "base/test/bind.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "gtest/gtest.h"
#include "net/base/url_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "services/network/test/test_url_loader_factory.h"
#include "base/memory/scoped_refptr.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "brave/components/ipfs/ipfs_utils.h"

namespace {

constexpr char kDefaultIpfsUrl [] = "ipfs://QmWiTTxzmTwHPoRsWczWJeNt8u5n3YzJBaWaUAwgmUxAEM";//"ipfs://bafybeiakou6e7hnx4ms2yangplzl6viapsoyo6phlee6bwrg4j2xt37m3q";

}  // namespace

class CarContentRequesterUnitTest : public testing::Test {
    public:
        CarContentRequesterUnitTest() = default;
        ~CarContentRequesterUnitTest() override = default;

    protected:
    void SetUp() override {
        TestingProfile::Builder builder;
        auto prefs =
            std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
        RegisterUserProfilePrefs(prefs->registry());
        builder.SetPrefService(std::move(prefs));
        profile_ = builder.Build();
    }

    PrefService* GetPrefs() { return profile_->GetPrefs(); }
    network::TestURLLoaderFactory* url_loader_factory() {
        return &url_loader_factory_;
    }
    base::test::TaskEnvironment* task_environment() { return &task_environment_; }

    private:
    content::BrowserTaskEnvironment task_environment_{base::test::TaskEnvironment::TimeSource::MOCK_TIME};
    std::unique_ptr<TestingProfile> profile_;
    network::TestURLLoaderFactory url_loader_factory_;
};

TEST_F(CarContentRequesterUnitTest, BasicTestSteps) {
    {
        auto request_callback = base::BindLambdaForTesting([&](std::unique_ptr<std::vector<uint8_t>> buffer, const bool is_success){
            EXPECT_TRUE(false) << "request_callback must not be called";
        });
        auto ccr = std::make_unique<ipfs::ipld::CarContentRequester>(GURL(""), 
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                url_loader_factory()), GetPrefs());
        ccr->Request(request_callback);
        EXPECT_FALSE(ccr->IsStarted());
    }

    {
        const uint64_t content_size = 1024;
        std::vector<char> content_data;
        for(uint64_t i=0; i < content_size; i++) { content_data.push_back('%'); }
        auto request_callback_counter = 0;
        auto request_callback = base::BindLambdaForTesting([&](std::unique_ptr<std::vector<uint8_t>> buffer, const bool is_success){
//            LOG(INFO) << "[IPFS] request_callback is_success:" << is_success << " buffer.size:" << buffer->size();
            request_callback_counter++;
            if(is_success){
                for(char ch : *buffer) { EXPECT_EQ(ch, '%'); }
            }
        });
        url_loader_factory()->SetInterceptor(
            base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
                LOG(INFO) << "[IPFS] Request url: " << request.url;
                ASSERT_TRUE(request.url.is_valid());
                ASSERT_TRUE(ipfs::IsDefaultGatewayURL(request.url, GetPrefs()));

                ASSERT_TRUE(base::Contains(request.url.query(), "format=car"));
                ASSERT_TRUE(base::Contains(request.url.query(), "dag-scope=entity"));
                ASSERT_TRUE(base::Contains(net::UnescapePercentEncodedUrl(request.url.query()), "entity-bytes=0:0"));

                auto response_head = network::mojom::URLResponseHead::New();
                response_head->headers =
                     base::MakeRefCounted<net::HttpResponseHeaders>("");
                response_head->headers->SetHeader("Content-Type", "application/vnd.ipld.car; version=1; order=dfs; dups=n");
                response_head->headers->ReplaceStatusLine("HTTP/1.1 200 OK");
                url_loader_factory()->AddResponse(
                     request.url, std::move(response_head), std::string(content_data.data(), content_data.size()),
                     network::URLLoaderCompletionStatus(net::OK));
        }));
        GURL url(kDefaultIpfsUrl);
        auto ccr = std::make_unique<ipfs::ipld::CarContentRequester>(url, base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                url_loader_factory()), GetPrefs());
        ccr->Request(request_callback);
        EXPECT_TRUE(ccr->IsStarted());
        task_environment()->RunUntilIdle();
        EXPECT_FALSE(ccr->IsStarted());
        EXPECT_EQ(request_callback_counter, 1);
    }
}

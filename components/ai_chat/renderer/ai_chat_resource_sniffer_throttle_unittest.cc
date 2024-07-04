// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/renderer/ai_chat_resource_sniffer.h"
#include "brave/components/ai_chat/renderer/ai_chat_resource_sniffer_throttle_delegate.h"
#include "brave/components/body_sniffer/body_sniffer_throttle.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_client.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

class MojoDataPipeSender {
 public:
  explicit MojoDataPipeSender(mojo::ScopedDataPipeProducerHandle handle)
      : handle_(std::move(handle)),
        watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC) {}

  void Start(std::string data, base::OnceClosure done_callback) {
    data_ = std::move(data);
    done_callback_ = std::move(done_callback);
    watcher_.Watch(handle_.get(),
                   MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
                   base::BindRepeating(&MojoDataPipeSender::OnWritable,
                                       base::Unretained(this)));
  }

  void OnWritable(MojoResult) {
    size_t sending_bytes = data_.size() - sent_bytes_;
    MojoResult result = handle_->WriteData(
        data_.c_str() + sent_bytes_, &sending_bytes, MOJO_WRITE_DATA_FLAG_NONE);
    switch (result) {
      case MOJO_RESULT_OK:
        break;
      case MOJO_RESULT_FAILED_PRECONDITION:
        // Finished unexpectedly.
        std::move(done_callback_).Run();
        return;
      case MOJO_RESULT_SHOULD_WAIT:
        // Just wait until OnWritable() is called by the watcher.
        return;
      default:
        NOTREACHED_IN_MIGRATION();
        return;
    }
    sent_bytes_ += sending_bytes;
    if (data_.size() == sent_bytes_) {
      std::move(done_callback_).Run();
    }
  }

  mojo::ScopedDataPipeProducerHandle ReleaseHandle() {
    return std::move(handle_);
  }

  bool has_succeeded() const { return data_.size() == sent_bytes_; }

 private:
  mojo::ScopedDataPipeProducerHandle handle_;
  mojo::SimpleWatcher watcher_;
  base::OnceClosure done_callback_;
  std::string data_;
  uint32_t sent_bytes_ = 0;
};

class MockAIChatResourceSnifferThrottleDelegate
    : public AIChatResourceSnifferThrottleDelegate {
 public:
  MOCK_METHOD(void, OnInterceptedPageContentChanged_Data, (std::string));

  void OnInterceptedPageContentChanged(
      std::unique_ptr<AIChatResourceSnifferThrottleDelegate::InterceptedContent>
          content) override {
    ASSERT_EQ(content->type,
              AIChatResourceSnifferThrottleDelegate::InterceptedContentType::
                  kYouTubeMetadataString);
    OnInterceptedPageContentChanged_Data(content->content);
  }

  base::WeakPtrFactory<MockAIChatResourceSnifferThrottleDelegate> weak_factory_{
      this};
};

class MockDelegate : public blink::URLLoaderThrottle::Delegate {
 public:
  // Implements blink::URLLoaderThrottle::Delegate.
  void CancelWithError(int error_code,
                       std::string_view custom_reason) override {
    NOTIMPLEMENTED();
  }
  void Resume() override {
    is_resumed_ = true;
    // Resume from OnReceiveResponse() with a customized response header.
    destination_loader_client()->OnReceiveResponse(
        std::move(updated_response_head_), std::move(body_), absl::nullopt);
  }

  void UpdateDeferredResponseHead(
      network::mojom::URLResponseHeadPtr new_response_head,
      mojo::ScopedDataPipeConsumerHandle body) override {
    updated_response_head_ = std::move(new_response_head);
    body_ = std::move(body);
  }
  void InterceptResponse(
      mojo::PendingRemote<network::mojom::URLLoader> new_loader,
      mojo::PendingReceiver<network::mojom::URLLoaderClient>
          new_client_receiver,
      mojo::PendingRemote<network::mojom::URLLoader>* original_loader,
      mojo::PendingReceiver<network::mojom::URLLoaderClient>*
          original_client_receiver,
      mojo::ScopedDataPipeConsumerHandle* body) override {
    is_intercepted_ = true;

    destination_loader_remote_.Bind(std::move(new_loader));
    ASSERT_TRUE(
        mojo::FusePipes(std::move(new_client_receiver),
                        mojo::PendingRemote<network::mojom::URLLoaderClient>(
                            destination_loader_client_.CreateRemote())));
    pending_receiver_ = original_loader->InitWithNewPipeAndPassReceiver();

    *original_client_receiver =
        source_loader_client_remote_.BindNewPipeAndPassReceiver();

    if (no_body_) {
      return;
    }

    DCHECK(!source_body_handle_);
    mojo::ScopedDataPipeConsumerHandle consumer;
    EXPECT_EQ(MOJO_RESULT_OK,
              mojo::CreateDataPipe(nullptr, source_body_handle_, consumer));
    *body = std::move(consumer);
  }

  void LoadResponseBody(const std::string& body) {
    MojoDataPipeSender sender(std::move(source_body_handle_));
    base::RunLoop loop;
    sender.Start(body, loop.QuitClosure());
    loop.Run();

    EXPECT_TRUE(sender.has_succeeded());
    source_body_handle_ = sender.ReleaseHandle();
  }

  void CompleteResponse() {
    source_loader_client_remote()->OnComplete(
        network::URLLoaderCompletionStatus());
    source_body_handle_.reset();
  }

  size_t ReadResponseBody(size_t size) {
    std::vector<uint8_t> buffer(size);
    MojoResult result = destination_loader_client_.response_body().ReadData(
        buffer.data(), &size, MOJO_READ_DATA_FLAG_NONE);
    switch (result) {
      case MOJO_RESULT_OK:
        return size;
      case MOJO_RESULT_FAILED_PRECONDITION:
        return 0;
      case MOJO_RESULT_SHOULD_WAIT:
        return 0;
      default:
        NOTREACHED_IN_MIGRATION();
    }
    return 0;
  }

  void ResetProducer() { source_body_handle_.reset(); }

  bool is_intercepted() const { return is_intercepted_; }
  bool is_resumed() const { return is_resumed_; }
  void set_no_body() { no_body_ = true; }

  network::TestURLLoaderClient* destination_loader_client() {
    return &destination_loader_client_;
  }

  mojo::Remote<network::mojom::URLLoaderClient>& source_loader_client_remote() {
    return source_loader_client_remote_;
  }

 private:
  bool is_intercepted_ = false;
  bool is_resumed_ = false;
  bool no_body_ = false;
  network::mojom::URLResponseHeadPtr updated_response_head_;
  mojo::ScopedDataPipeConsumerHandle body_;

  // A pair of a loader and a loader client for destination of the response.
  mojo::Remote<network::mojom::URLLoader> destination_loader_remote_;
  network::TestURLLoaderClient destination_loader_client_;

  // A pair of a receiver and a remote for source of the response.
  mojo::PendingReceiver<network::mojom::URLLoader> pending_receiver_;
  mojo::Remote<network::mojom::URLLoaderClient> source_loader_client_remote_;

  mojo::ScopedDataPipeProducerHandle source_body_handle_;
};

}  // namespace

class AIChatResourceSnifferThrottleTest : public testing::Test {
 public:
  std::unique_ptr<blink::URLLoaderThrottle> MaybeCreateThrottleForUrl(
      GURL url) {
    auto ai_chat_resource_sniffer = AIChatResourceSniffer::MaybeCreate(
        url, ai_chat_throttle_delegate_.weak_factory_.GetWeakPtr());
    if (ai_chat_resource_sniffer) {
      auto body_sniffer_throttle =
          std::make_unique<body_sniffer::BodySnifferThrottle>(
              task_environment_.GetMainThreadTaskRunner());
      body_sniffer_throttle->AddHandler(std::move(ai_chat_resource_sniffer));
      return body_sniffer_throttle;
    }
    return nullptr;
  }

  void InterceptBodyRequestFor(const std::string& body) {
    GURL url("https://www.youtube.com/youtubei/v1/player");
    auto throttle = MaybeCreateThrottleForUrl(url);
    auto delegate = std::make_unique<MockDelegate>();
    throttle->set_delegate(delegate.get());

    auto response_head = network::mojom::URLResponseHead::New();
    bool defer = false;
    throttle->WillProcessResponse(url, response_head.get(), &defer);
    EXPECT_FALSE(defer);
    EXPECT_TRUE(delegate->is_intercepted());

    delegate->LoadResponseBody(body);
    delegate->CompleteResponse();
    task_environment_.RunUntilIdle();
    EXPECT_TRUE(delegate->destination_loader_client()->has_received_response());
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  testing::NiceMock<MockAIChatResourceSnifferThrottleDelegate>
      ai_chat_throttle_delegate_;
};

TEST_F(AIChatResourceSnifferThrottleTest, ThrottlesYTPlayerAPI) {
  EXPECT_NE(nullptr, MaybeCreateThrottleForUrl(GURL(
                         "http://www.youtube.com/youtubei/v1/player?example")));
}

TEST_F(AIChatResourceSnifferThrottleTest, DoesNotThrottleYTOther) {
  EXPECT_EQ(nullptr,
            MaybeCreateThrottleForUrl(GURL(
                "http://www.youtube.com/youtubei/v1/somethingelse?example")));
}

TEST_F(AIChatResourceSnifferThrottleTest, DoesNotThrottleNonYT) {
  EXPECT_EQ(nullptr, MaybeCreateThrottleForUrl(GURL(
                         "http://www.example.com/youtubei/v1/player?example")));
}

TEST_F(AIChatResourceSnifferThrottleTest, DoesNotThrottleNonHTTP) {
  EXPECT_EQ(nullptr, MaybeCreateThrottleForUrl(GURL(
                         "wss://www.youtube.com/youtubei/v1/player?example")));
}

TEST_F(AIChatResourceSnifferThrottleTest, Body_NonJson) {
  // AIChatResourceSnifferThrottle doesn't parse the json as an optimization
  // since it might not get used until an AIChat conversation message is about
  // to be sent, so any body content should be passed to the delegate, we don't
  // need to test for valid JSON
  std::string body = "\x89PNG\x0D\x0A\x1A\x0A";
  EXPECT_CALL(ai_chat_throttle_delegate_,
              OnInterceptedPageContentChanged_Data(body))
      .Times(1);
  InterceptBodyRequestFor(body);
}

TEST_F(AIChatResourceSnifferThrottleTest, Body_ValidYTJson) {
  std::string body = R"({
    "captions": {
      "playerCaptionsTracklistRenderer": {
        "captionTracks": [
          {
            "baseUrl": "https://www.example.com/caption1"
          }
        ]
      }
    }
  })";
  EXPECT_CALL(ai_chat_throttle_delegate_,
              OnInterceptedPageContentChanged_Data(body))
      .Times(1);
  InterceptBodyRequestFor(body);
}

TEST_F(AIChatResourceSnifferThrottleTest, LongBody) {
  std::string body = "This should be long enough...";
  body.resize(2048, 'a');
  EXPECT_CALL(ai_chat_throttle_delegate_,
              OnInterceptedPageContentChanged_Data(body))
      .Times(1);
  InterceptBodyRequestFor(body);
}

}  // namespace ai_chat

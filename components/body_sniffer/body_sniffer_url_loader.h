/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BODY_SNIFFER_BODY_SNIFFER_URL_LOADER_H_
#define BRAVE_COMPONENTS_BODY_SNIFFER_BODY_SNIFFER_URL_LOADER_H_

#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/request_priority.h"
#include "services/network/public/mojom/early_hints.mojom-forward.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace net {
class HttpRequestHeaders;
struct RedirectInfo;
}  // namespace net

namespace network {
struct URLLoaderCompletionStatus;
}  // namespace network

namespace body_sniffer {

class BodySnifferThrottle;

class BodySnifferURLLoader : public network::mojom::URLLoaderClient,
                             public network::mojom::URLLoader {
 public:
  ~BodySnifferURLLoader() override;

  BodySnifferURLLoader(const BodySnifferURLLoader&) = delete;
  BodySnifferURLLoader& operator=(const BodySnifferURLLoader&) = delete;

  // Start waiting for the body.
  void Start(
      mojo::PendingRemote<network::mojom::URLLoader> source_url_loader_remote,
      mojo::PendingReceiver<network::mojom::URLLoaderClient>
          source_url_client_receiver,
      mojo::ScopedDataPipeConsumerHandle body);

  mojo::ScopedDataPipeConsumerHandle* GetNextConsumerHandle() {
    return &next_body_consumer_handle_;
  }

 protected:
  BodySnifferURLLoader(
      base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
      const GURL& response_url,
      mojo::PendingRemote<network::mojom::URLLoaderClient>
          destination_url_loader_client,
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  // network::mojom::URLLoaderClient implementation (called from the source of
  // the response):
  void OnReceiveEarlyHints(network::mojom::EarlyHintsPtr early_hints) override;
  void OnReceiveResponse(
      network::mojom::URLResponseHeadPtr response_head,
      mojo::ScopedDataPipeConsumerHandle body,
      absl::optional<mojo_base::BigBuffer> cached_metadata) override;
  void OnReceiveRedirect(
      const net::RedirectInfo& redirect_info,
      network::mojom::URLResponseHeadPtr response_head) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnComplete(const network::URLLoaderCompletionStatus& status) override;

  // network::mojom::URLLoader implementation (called from the destination of
  // the response):
  void FollowRedirect(
      const std::vector<std::string>& removed_headers,
      const net::HttpRequestHeaders& modified_headers,
      const net::HttpRequestHeaders& modified_cors_exempt_headers,
      const absl::optional<GURL>& new_url) override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  bool CheckBufferedBody(uint32_t readBufferSize);

  virtual void OnBodyReadable(MojoResult) = 0;
  virtual void OnBodyWritable(MojoResult) = 0;

  virtual void CompleteLoading(std::string body);
  void CompleteSending();
  virtual void OnCompleteSending();
  void SendBufferedBodyToClient();

  void Abort();

  base::WeakPtr<BodySnifferThrottle> throttle_;
  const GURL response_url_;

  mojo::Receiver<network::mojom::URLLoaderClient> source_url_client_receiver_{
      this};
  mojo::Remote<network::mojom::URLLoader> source_url_loader_;
  mojo::Remote<network::mojom::URLLoaderClient> destination_url_loader_client_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  enum class State { kWaitForBody, kLoading, kSending, kCompleted, kAborted };
  State state_ = State::kWaitForBody;

  absl::optional<network::URLLoaderCompletionStatus> complete_status_;

  std::string buffered_body_;
  size_t bytes_remaining_in_buffer_;
  size_t read_bytes_ = 0;

  mojo::ScopedDataPipeConsumerHandle body_consumer_handle_;
  mojo::ScopedDataPipeProducerHandle body_producer_handle_;
  mojo::SimpleWatcher body_consumer_watcher_;
  mojo::SimpleWatcher body_producer_watcher_;
  mojo::ScopedDataPipeConsumerHandle next_body_consumer_handle_;

 private:
  void CancelAndResetHandles();

  base::WeakPtrFactory<BodySnifferURLLoader> weak_factory_{this};
};

}  // namespace body_sniffer

#endif  // BRAVE_COMPONENTS_BODY_SNIFFER_BODY_SNIFFER_URL_LOADER_H_

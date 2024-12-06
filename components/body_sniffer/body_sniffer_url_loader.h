/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BODY_SNIFFER_BODY_SNIFFER_URL_LOADER_H_
#define BRAVE_COMPONENTS_BODY_SNIFFER_BODY_SNIFFER_URL_LOADER_H_

#include <memory>
#include <optional>
#include <string>
#include <tuple>
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
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "url/gurl.h"

namespace net {
class HttpRequestHeaders;
struct RedirectInfo;
}  // namespace net

namespace network {
struct URLLoaderCompletionStatus;
struct ResourceRequest;
}  // namespace network

namespace body_sniffer {

// An interface for handlers that require the original content of the page.
class BodyHandler {
 public:
  enum class Action {
    // Default value, shouldn be never returned from OnBodyUpdated.
    kNone,
    // The handler needs more data.
    kContinue,
    // The handler has received enough data.
    // Completed handlers are stopped to receive data updates notifications.
    kComplete,
    // The handler decides to cancel the loading.
    kCancel,
    // The handler decides to abort the loading.
    kAbort,
  };

  virtual ~BodyHandler() = default;

  // Called on request start, returns true if request is interesting for this
  // handler.
  virtual bool OnRequest(network::ResourceRequest* request) = 0;

  // Called on response received, returns true if the handler wants to process
  // this response. |defer| assignment is ignored if returns false.
  virtual bool ShouldProcess(const GURL& response_url,
                             network::mojom::URLResponseHead* response_head,
                             bool* defer) = 0;

  // Called before sending the content to the consumer.
  virtual void OnBeforeSending() = 0;

  // Called when the page content reaches the consumer.
  virtual void OnComplete() = 0;

  // Called every time when url loader receives a chunk of data.
  // is_complete is true when the last chunk is received.
  virtual Action OnBodyUpdated(const std::string& body, bool is_complete) = 0;

  // If it returns true, the handler needs the entire page content to transform
  // it (e.g. Distill).
  virtual bool IsTransformer() const = 0;

  // Called only if the IsTransformer() returns true, all transformers
  // receive the |body| in the order of creation. First one gets the original
  // body, the second one will get the result of the first and so on.
  virtual void Transform(std::string body,
                         base::OnceCallback<void(std::string)> on_complete) = 0;

  // Updates the response head.
  virtual void UpdateResponseHead(
      network::mojom::URLResponseHead* response_head) = 0;
};

// An interface for handlers that completely replace the original content of the
// page.
class BodyProducer {
 public:
  virtual ~BodyProducer() = default;

  // Updates the original response head.
  virtual void UpdateResponseHead(
      network::mojom::URLResponseHead* response_head) = 0;
  // Content to be sent to the consumer.
  virtual std::string TakeContent() = 0;
  // Called before sending the content to the consumer.
  virtual void OnBeforeSending() = 0;
  // Called when the content reaches the consumer.
  virtual void OnComplete() = 0;
};

class BodySnifferThrottle;

using BodyHandlersPtr = std::vector<std::unique_ptr<BodyHandler>>;
using BodyProducerPtr = std::unique_ptr<BodyProducer>;
using Handler = absl::variant<BodyProducerPtr, BodyHandlersPtr>;

// When created with BodyHandlersPtr the BodySnifferURLLoader continuously
// receives the content of the page and passes every chunk to handlers. If all
// handlers decide to kComplete (or the entire body is received) then
// BodySnifferURLLoader stops sniffing and forwards the content to the consumer.
//
// When created with BodyProducerPtr the BodySnifferURLLoader stops the original
// producer and forwards producer's content to the consumer.
class BodySnifferURLLoader : public network::mojom::URLLoaderClient,
                             public network::mojom::URLLoader {
 public:
  BodySnifferURLLoader(const BodySnifferURLLoader&) = delete;
  BodySnifferURLLoader& operator=(const BodySnifferURLLoader&) = delete;
  ~BodySnifferURLLoader() override;

  // Start waiting for the body.
  void Start(
      mojo::PendingRemote<network::mojom::URLLoader> source_url_loader_remote,
      mojo::PendingReceiver<network::mojom::URLLoaderClient>
          source_url_client_receiver,
      mojo::ScopedDataPipeConsumerHandle body);

  // mojo::PendingRemote<network::mojom::URLLoader> controls the lifetime of the
  // loader.
  static std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
                    mojo::PendingReceiver<network::mojom::URLLoaderClient>,
                    BodySnifferURLLoader*,
                    mojo::ScopedDataPipeConsumerHandle>
  CreateLoader(base::WeakPtr<BodySnifferThrottle> throttle,
               network::mojom::URLResponseHeadPtr response_head,
               Handler handler,
               scoped_refptr<base::SequencedTaskRunner> task_runner);

 private:
  BodySnifferURLLoader(base::WeakPtr<BodySnifferThrottle> throttle,
                       network::mojom::URLResponseHeadPtr response_head,
                       Handler handler,
                       mojo::PendingRemote<network::mojom::URLLoaderClient>
                           destination_url_loader_client,
                       scoped_refptr<base::SequencedTaskRunner> task_runner);

  // network::mojom::URLLoaderClient implementation (called from the source of
  // the response):
  void OnReceiveEarlyHints(network::mojom::EarlyHintsPtr early_hints) override;
  void OnReceiveResponse(
      network::mojom::URLResponseHeadPtr response_head,
      mojo::ScopedDataPipeConsumerHandle body,
      std::optional<mojo_base::BigBuffer> cached_metadata) override;
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
      const std::optional<GURL>& new_url) override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  void OnBodyReadable(MojoResult);
  void OnBodyWritable(MojoResult);
  void CompleteSniffing(bool remove_first, std::string body);
  void CompleteSending();
  void SendReceivedBodyToClient();
  void ForwardBodyToClient();
  void Cancel();
  void Abort();

  base::WeakPtr<BodySnifferThrottle> throttle_;

  mojo::Receiver<network::mojom::URLLoaderClient> source_url_client_receiver_{
      this};
  mojo::Remote<network::mojom::URLLoader> source_url_loader_;
  mojo::Remote<network::mojom::URLLoaderClient> destination_url_loader_client_;

  // Capture the response head to defer to send it to the destination.
  network::mojom::URLResponseHeadPtr response_head_;

  Handler handler_;
  BodyHandlersPtr complete_handlers_;

  enum class State { kWaitForBody, kSniffing, kSending, kCompleted, kAborted };
  State state_ = State::kWaitForBody;

  // Set if OnComplete() is called during sniffing.
  std::optional<network::URLLoaderCompletionStatus> complete_status_;

  std::string buffered_body_;
  size_t bytes_remaining_in_buffer_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  mojo::ScopedDataPipeConsumerHandle body_consumer_handle_;
  mojo::ScopedDataPipeProducerHandle body_producer_handle_;
  mojo::SimpleWatcher body_consumer_watcher_;
  mojo::SimpleWatcher body_producer_watcher_;

  base::WeakPtrFactory<BodySnifferURLLoader> weak_factory_{this};
};

}  // namespace body_sniffer

#endif  // BRAVE_COMPONENTS_BODY_SNIFFER_BODY_SNIFFER_URL_LOADER_H_

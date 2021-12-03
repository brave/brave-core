/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_URL_LOADER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_URL_LOADER_H_

#include <string>
#include <tuple>
#include <vector>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_piece.h"
#include "brave/components/speedreader/speedreader_result_delegate.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "services/network/public/mojom/early_hints.mojom-forward.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace speedreader {

class SpeedReaderThrottle;
class SpeedreaderRewriterService;

// Loads the whole response body and tries to Speedreader-distill it.
// Cargoculted from |`SniffingURLLoader|.
//
// This loader has five states:
// kWaitForBody: The initial state until the body is received (=
//               OnStartLoadingResponseBody() is called) or the response is
//               finished (= OnComplete() is called). When body is provided, the
//               state is changed to kLoading. Otherwise the state goes to
//               kCompleted.
// kLoading: Receives the body from the source loader and distills the page.
//            The received body is kept in this loader until distilling
//            is finished. When all body has been received and distilling is
//            done, this loader will dispatch queued messages like
//            OnStartLoadingResponseBody() to the destination
//            loader client, and then the state is changed to kSending.
// kSending: Receives the body and sends it to the destination loader client.
//           The state changes to kCompleted after all data is sent.
// kCompleted: All data has been sent to the destination loader.
// kAborted: Unexpected behavior happens. Watchers, pipes and the binding from
//           the source loader to |this| are stopped. All incoming messages from
//           the destination (through network::mojom::URLLoader) are ignored in
class SpeedReaderURLLoader : public network::mojom::URLLoaderClient,
                             public network::mojom::URLLoader {
 public:
  ~SpeedReaderURLLoader() override;

  SpeedReaderURLLoader(const SpeedReaderURLLoader&) = delete;
  SpeedReaderURLLoader& operator=(const SpeedReaderURLLoader&) = delete;

  // Start waiting for the body.
  void Start(
      mojo::PendingRemote<network::mojom::URLLoader> source_url_loader_remote,
      mojo::PendingReceiver<network::mojom::URLLoaderClient>
          source_url_client_receiver);

  // mojo::PendingRemote<network::mojom::URLLoader> controls the lifetime of the
  // loader.
  static std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
                    mojo::PendingReceiver<network::mojom::URLLoaderClient>,
                    SpeedReaderURLLoader*>
  CreateLoader(base::WeakPtr<SpeedReaderThrottle> throttle,
               base::WeakPtr<SpeedreaderResultDelegate> delegate,
               const GURL& response_url,
               scoped_refptr<base::SingleThreadTaskRunner> task_runner,
               SpeedreaderRewriterService* rewriter_service);

 private:
  SpeedReaderURLLoader(base::WeakPtr<SpeedReaderThrottle> throttle,
                       base::WeakPtr<SpeedreaderResultDelegate> delegate,
                       const GURL& response_url,
                       mojo::PendingRemote<network::mojom::URLLoaderClient>
                           destination_url_loader_client,
                       scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                       SpeedreaderRewriterService* rewriter_service);

  // network::mojom::URLLoaderClient implementation (called from the source of
  // the response):
  void OnReceiveEarlyHints(network::mojom::EarlyHintsPtr early_hints) override;
  void OnReceiveResponse(
      network::mojom::URLResponseHeadPtr response_head) override;
  void OnReceiveRedirect(
      const net::RedirectInfo& redirect_info,
      network::mojom::URLResponseHeadPtr response_head) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override;
  void OnReceiveCachedMetadata(mojo_base::BigBuffer data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
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

  void OnBodyReadable(MojoResult);
  void OnBodyWritable(MojoResult);
  void MaybeLaunchSpeedreader();

  // Gets either distilled or untouched body.
  void CompleteLoading(std::string body);
  void CompleteSending();
  void SendReceivedBodyToClient();

  void Abort();

  base::WeakPtr<SpeedReaderThrottle> throttle_;
  base::WeakPtr<SpeedreaderResultDelegate> delegate_;

  mojo::Receiver<network::mojom::URLLoaderClient> source_url_client_receiver_{
      this};
  mojo::Remote<network::mojom::URLLoader> source_url_loader_;
  mojo::Remote<network::mojom::URLLoaderClient> destination_url_loader_client_;

  GURL response_url_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  enum class State { kWaitForBody, kLoading, kSending, kCompleted, kAborted };
  State state_ = State::kWaitForBody;

  // Set if OnComplete() is called during distilling.
  absl::optional<network::URLLoaderCompletionStatus> complete_status_;

  // Note that this could be replaced by a distilled version.
  std::string buffered_body_;
  size_t bytes_remaining_in_buffer_;

  mojo::ScopedDataPipeConsumerHandle body_consumer_handle_;
  mojo::ScopedDataPipeProducerHandle body_producer_handle_;
  mojo::SimpleWatcher body_consumer_watcher_;
  mojo::SimpleWatcher body_producer_watcher_;

  // Not Owned
  raw_ptr<SpeedreaderRewriterService> rewriter_service_ = nullptr;

  base::WeakPtrFactory<SpeedReaderURLLoader> weak_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_URL_LOADER_H_

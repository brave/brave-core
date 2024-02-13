/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_LOCAL_URL_LOADER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_LOCAL_URL_LOADER_H_

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "brave/components/body_sniffer/body_sniffer_throttle.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace speedreader {

class SpeedreaderThrottleDelegate;

// Local url loader is a content source that replaces network source, it takes
// the content of the pre-distilled page |body_content| and sends it to the
// consumer.
class SpeedReaderLocalURLLoader : public network::mojom::URLLoaderClient,
                                  public network::mojom::URLLoader {
 public:
  ~SpeedReaderLocalURLLoader() override;

  // mojo::PendingRemote<network::mojom::URLLoader> controls the lifetime of the
  // loader.
  static std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
                    mojo::PendingReceiver<network::mojom::URLLoaderClient>,
                    SpeedReaderLocalURLLoader*>
  CreateLoader(base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
               base::WeakPtr<SpeedreaderThrottleDelegate> delegate,
               scoped_refptr<base::SequencedTaskRunner> task_runner);

  mojo::ScopedDataPipeConsumerHandle* GetDestinationConsumerHandle();

  // Start loader and send |body_content| to the next loader.
  void Start(std::string body_content);

 private:
  SpeedReaderLocalURLLoader(
      base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
      base::WeakPtr<SpeedreaderThrottleDelegate> delegate,
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

  void OnBodyWritable(MojoResult);
  void CompleteSending();
  void SendBodyToClient();
  void Abort();

  base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle_;
  base::WeakPtr<SpeedreaderThrottleDelegate> delegate_;
  mojo::Remote<network::mojom::URLLoaderClient> destination_url_loader_client_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  std::string body_content_;
  size_t bytes_remaining_in_body_content_;

  mojo::ScopedDataPipeConsumerHandle destination_consumer_handle_;
  mojo::ScopedDataPipeProducerHandle body_producer_handle_;
  mojo::SimpleWatcher body_producer_watcher_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_LOCAL_URL_LOADER_H_

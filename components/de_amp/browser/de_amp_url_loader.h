/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_URL_LOADER_H_
#define BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_URL_LOADER_H_

#include <string>
#include <tuple>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/de_amp/browser/de_amp_service.h"
#include "content/public/browser/web_contents.h"
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

namespace de_amp {

class DeAmpThrottle;

class DeAmpURLLoader : public network::mojom::URLLoaderClient,
                       public network::mojom::URLLoader {
 public:
  ~DeAmpURLLoader() override;

  DeAmpURLLoader(const DeAmpURLLoader&) = delete;
  DeAmpURLLoader& operator=(const DeAmpURLLoader&) = delete;

  // Start waiting for the body.
  void Start(
      mojo::PendingRemote<network::mojom::URLLoader> source_url_loader_remote,
      mojo::PendingReceiver<network::mojom::URLLoaderClient>
          source_url_client_receiver);

  // mojo::PendingRemote<network::mojom::URLLoader> controls the lifetime of the
  // loader.
  static std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
                    mojo::PendingReceiver<network::mojom::URLLoaderClient>,
                    DeAmpURLLoader*>
  CreateLoader(base::WeakPtr<DeAmpThrottle> throttle,
               const GURL& response_url,
               scoped_refptr<base::SequencedTaskRunner> task_runner,
               DeAmpService* service,
               content::WebContents* contents);

 private:
  DeAmpURLLoader(base::WeakPtr<DeAmpThrottle> throttle,
                 const GURL& response_url,
                 mojo::PendingRemote<network::mojom::URLLoaderClient>
                     destination_url_loader_client,
                 scoped_refptr<base::SequencedTaskRunner> task_runner,
                 DeAmpService* service,
                 content::WebContents* contents);

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

  void CompleteLoading(std::string body);
  void CompleteSending();
  void ForwardBodyToClient();
  void SendReceivedBodyToClient();

  void Abort();

  base::WeakPtr<DeAmpThrottle> throttle_;

  mojo::Receiver<network::mojom::URLLoaderClient> source_url_client_receiver_{
      this};
  mojo::Remote<network::mojom::URLLoader> source_url_loader_;
  mojo::Remote<network::mojom::URLLoaderClient> destination_url_loader_client_;

  GURL response_url_;
  content::WebContents* contents_;
  DeAmpService* de_amp_service;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  enum class State { kWaitForBody, kLoading, kSending, kCompleted, kAborted };
  State state_ = State::kWaitForBody;

  absl::optional<network::URLLoaderCompletionStatus> complete_status_;

  std::string buffered_body_;
  size_t bytes_remaining_in_buffer_;

  mojo::ScopedDataPipeConsumerHandle body_consumer_handle_;
  mojo::ScopedDataPipeProducerHandle body_producer_handle_;
  mojo::SimpleWatcher body_consumer_watcher_;
  mojo::SimpleWatcher body_producer_watcher_;

  base::WeakPtrFactory<DeAmpURLLoader> weak_factory_{this};
};

}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_URL_LOADER_H_

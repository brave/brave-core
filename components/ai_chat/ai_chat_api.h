/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_API_H_
#define BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_API_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/mojom/json_parser.mojom.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class AIChatAPI : public network::SimpleURLLoaderStreamConsumer {
 public:
  explicit AIChatAPI(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  AIChatAPI(const AIChatAPI&) = delete;
  AIChatAPI& operator=(const AIChatAPI&) = delete;
  ~AIChatAPI() override;

  using ResponseCallback = base::RepeatingCallback<void(const std::string&)>;
  using CompletionCallback =
      base::OnceCallback<void(bool success, int response_code)>;

  bool IsRequestInProgress();
  void QueryPrompt(ResponseCallback response_callback,
                   CompletionCallback completion_callback,
                   const std::string& prompt);

  void SetResponseCallbackForTesting(ResponseCallback response_callback) {
    response_callback_ = response_callback;
  }

  void SendDataForTesting(const std::string& text) {
    OnDataReceived(text, base::BindOnce([]() {}));
  }

 private:
  base::Value::Dict CreateApiParametersDict(const std::string& prompt);

  // network::SimpleURLLoaderStreamConsumer implementation:
  void OnDataReceived(base::StringPiece string_piece,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override;

  void OnParseJsonIsolated(data_decoder::DataDecoder::ValueOrError result);
  void OnResponseStarted(const GURL& final_url,
                         const network::mojom::URLResponseHead& response_head);
  void OnDownloadProgress(uint64_t current);

  ResponseCallback response_callback_;
  CompletionCallback completion_callback_;

  api_request_helper::APIRequestHelper::Ticket current_request_{};
  api_request_helper::APIRequestHelper api_request_helper_;

  std::unique_ptr<data_decoder::DataDecoder> data_decoder_;
  mojo::Remote<data_decoder::mojom::JsonParser> json_parser_;

  bool is_request_in_progress_{false};

  base::WeakPtrFactory<AIChatAPI> weak_ptr_factory_{this};
};

#endif  // BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_API_H_

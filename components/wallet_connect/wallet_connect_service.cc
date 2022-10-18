/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/wallet_connect/wallet_connect_service.h"

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/wallet_connect/encryptor.h"
#include "brave/components/wallet_connect/wallet_connect.h"
#include "brave/components/wallet_connect/wallet_connect_utils.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_constants.h"
#include "services/cert_verifier/public/mojom/cert_verifier_service_factory.mojom.h"
#include "url/url_constants.h"

namespace wallet_connect {

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("wallet_connect_service", R"(
        semantics {
          sender: "Wallet Connect Service"
          description:
            "Brave use this web socket connection to communicate to the "
            "Wallet Connect bridge server which is a rendezvous service for "
            "dapps and Brave Wallet. Users will use wallet connect uri "
            "produced by dapps and paste it to wallet or scan the QR code "
            "which contains the uri. And then Brave Wallet will use the uri to "
            "connect to bridge server."
          trigger:
            "Users initite the connection by providing a valid Wallet Connect
            "uri"
          data:
            "Initial handshake is plaintext which doesn't contain any keys."
            "After session is established, the payload will be encrypted by "
            "the key specified in Wallet Connect uri."
          destination: OTHER
        }
        policy {
          cookies_allowed: NO
          setting: "Not controlled by a setting because the operation is "
            "triggered by significant user action."
          policy_exception_justification:
            "No policy provided because the operation is triggered by "
            " significant user action. No background activity occurs."
        })");

WalletConnectService::WalletConnectService()
    : client_id_(base::GenerateGUID()) {}

WalletConnectService::~WalletConnectService() = default;

mojo::PendingRemote<mojom::WalletConnectService>
WalletConnectService::MakeRemote() {
  mojo::PendingRemote<mojom::WalletConnectService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void WalletConnectService::Bind(
    mojo::PendingReceiver<mojom::WalletConnectService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void WalletConnectService::Init(const std::string& wc_uri,
                                InitCallback callback) {
  auto data = ParseWalletConnectURI(wc_uri);
  if (!data) {
    std::move(callback).Run(false);
    return;
  }
  wallet_connect_uri_data_ = std::move(data);

  DCHECK(wallet_connect_uri_data_->params &&
         wallet_connect_uri_data_->params->is_v1_params());
  const auto& v1_params = wallet_connect_uri_data_->params->get_v1_params();
  GURL url = v1_params->bridge;
  if (!url.SchemeIsWSOrWSS()) {
    GURL::Replacements scheme_replacements;
    scheme_replacements.SetSchemeStr(url::kWssScheme);
    url = url.ReplaceComponents(scheme_replacements);
  }

  auto context_params = network::mojom::NetworkContextParams::New();
  context_params->cert_verifier_params = content::GetCertVerifierParams(
      cert_verifier::mojom::CertVerifierCreationParams::New());
  context_params->cors_exempt_header_list = {
      content::kCorsExemptPurposeHeaderName};
  context_params->cookie_manager_params =
      network::mojom::CookieManagerParams::New();

  context_params->http_cache_enabled = false;
  context_params->enable_certificate_reporting = false;
  context_params->enable_expect_ct_reporting = false;
  context_params->enable_domain_reliability = false;

  if (!network_context_.is_bound())
    content::CreateNetworkContextInNetworkService(
        network_context_.BindNewPipeAndPassReceiver(),
        std::move(context_params));

  websocket_client_ = std::make_unique<WebSocketAdapter>(
      base::BindOnce(&WalletConnectService::OnTunnelReady,
                     base::Unretained(this)),
      base::BindRepeating(&WalletConnectService::OnTunnelData,
                          base::Unretained(this)));

  network_context_->CreateWebSocket(
      url, {}, net::SiteForCookies(), net::IsolationInfo(),
      /*additional_headers=*/{}, network::mojom::kBrowserProcessId,
      url::Origin::Create(url), network::mojom::kWebSocketOptionBlockAllCookies,
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation),
      websocket_client_->BindNewHandshakeClientPipe(),
      /*url_loader_network_observer=*/mojo::NullRemote(),
      /*auth_handler=*/mojo::NullRemote(),
      /*header_client=*/mojo::NullRemote(),
      /*throttling_profile_id=*/absl::nullopt);

  std::move(callback).Run(true);
}

void WalletConnectService::OnTunnelReady(bool success) {
  if (success) {
    state_ = State::kConnected;
    // subscribe to handshae topic
    types::SocketMessage message;
    message.topic = wallet_connect_uri_data_->topic;
    message.type = "sub";
    message.payload = "";
    message.silent = true;
    std::string json;
    if (base::JSONWriter::Write(message.ToValue(), &json)) {
      LOG(ERROR) << "send: " << json;
      websocket_client_->Write(std::vector<uint8_t>(json.begin(), json.end()));
    }
    // subscribe to own client id
    message.topic = client_id_;
    if (base::JSONWriter::Write(message.ToValue(), &json)) {
      LOG(ERROR) << "send: " << json;
      websocket_client_->Write(std::vector<uint8_t>(json.begin(), json.end()));
    }
  }
}

void WalletConnectService::OnTunnelData(
    absl::optional<base::span<const uint8_t>> data) {
  if (data) {
    const std::string data_str((const char*)data->data(), data->size());
    LOG(ERROR) << "receive: " << data_str;
    // TODO: sanitize input
    auto value = base::JSONReader::Read(data_str);
    if (!value) {
      return;
    }
    auto message = types::SocketMessage::FromValue(*value);
    // ack
    types::SocketMessage ack_msg;
    ack_msg.topic = message->topic;
    ack_msg.type = "ack";
    ack_msg.payload = "";
    ack_msg.silent = true;
    std::string ack_json;
    if (base::JSONWriter::Write(ack_msg.ToValue(), &ack_json)) {
      LOG(ERROR) << "send: " << ack_json;
      websocket_client_->Write(
          std::vector<uint8_t>(ack_json.begin(), ack_json.end()));
    }

    // decrypting message
    std::vector<uint8_t> key_vec;
    DCHECK(wallet_connect_uri_data_->params &&
           wallet_connect_uri_data_->params->is_v1_params());
    if (!base::HexStringToBytes(
            wallet_connect_uri_data_->params->get_v1_params()->key, &key_vec)) {
      return;
    }
    std::array<uint8_t, 32> key;
    std::copy_n(key_vec.begin(), 32, key.begin());
    Encryptor encryptor(key);
    auto ciphertext_value = base::JSONReader::Read(message->payload);
    if (!ciphertext_value) {
      return;
    }
    auto ciphertext = types::EncryptionPayload::FromValue(*ciphertext_value);
    if (!ciphertext)
      return;
    auto decrypted_payload = encryptor.Decrypt(*ciphertext);
    if (!decrypted_payload.has_value()) {
      LOG(ERROR) << decrypted_payload.error();
    }
    auto decrypted_payload_value = decrypted_payload.value();
    const std::string decrypted_payload_value_str(
        decrypted_payload_value.begin(), decrypted_payload_value.end());
    LOG(ERROR) << "decrypted: " << decrypted_payload_value_str;

    // extract rpc request
    auto rpc_request_value =
        base::JSONReader::Read(decrypted_payload_value_str);
    if (!rpc_request_value) {
      LOG(ERROR) << "can't read rpc request json";
      return;
    }
    auto rpc_request = types::JsonRpcRequest::FromValue(*rpc_request_value);
    if (!rpc_request) {
      LOG(ERROR) << "rpc request from value failed"
                 << rpc_request_value->DebugString();
      return;
    }

    if (state_ == State::kConnected) {
      // extract session request
      DCHECK(rpc_request->method == "wc_sessionRequest");
      DCHECK(rpc_request->params.size() == 1);
      auto session_request =
          types::SessionRequest::FromValue(std::move(rpc_request->params[0]));
      if (!session_request) {
        LOG(ERROR) << "session request from value failed";
        return;
      }

      // construct approved session_params
      types::SessionParams session_params;
      session_params.approved = true;
      session_params.chain_id = 1;
      session_params.network_id = 0;
      session_params.accounts.push_back(
          "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
      session_params.peer_id = client_id_;
      session_params.rpc_url = absl::nullopt;
      types::ClientMeta meta;
      meta.name = "Brave Wallet";
      session_params.peer_meta = std::move(meta);

      // put session_params into JsonRpcResponseSuccess
      types::JsonRpcResponseSuccess response;
      response.id = rpc_request->id;
      response.jsonrpc = "2.0";
      response.result = base::Value(session_params.ToValue());

      std::string response_json;
      if (!base::JSONWriter::Write(response.ToValue(), &response_json)) {
        return;
      }
      LOG(ERROR) << "encrypting: " << response_json;
      auto encrypted_payload = encryptor.Encrypt(
          std::vector<uint8_t>(response_json.begin(), response_json.end()));
      DCHECK(encrypted_payload.has_value());
      std::string encrypted_response_json;
      if (!base::JSONWriter::Write(encrypted_payload.value().ToValue(),
                                   &encrypted_response_json)) {
        return;
      }
      // put encrypted payload into socket message
      types::SocketMessage socket_response;
      socket_response.topic = session_request->peer_id;
      socket_response.type = "pub";
      socket_response.payload = encrypted_response_json;
      socket_response.silent = true;

      std::string socket_response_json;
      if (!base::JSONWriter::Write(socket_response.ToValue(),
                                   &socket_response_json)) {
        return;
      }
      LOG(ERROR) << "send: " << socket_response_json;
      websocket_client_->Write(std::vector<uint8_t>(
          socket_response_json.begin(), socket_response_json.end()));
      state_ = State::kSessionEstablished;
    } else if (state_ == State::kSessionEstablished) {
      // Handle wallet request
      LOG(ERROR) << *rpc_request_value;
    }
  }
}

}  // namespace wallet_connect

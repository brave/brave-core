/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_star_manager.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/components/p3a/p3a_message.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave {

namespace {

constexpr std::size_t kP3AStarCurrentThreshold = 50;
constexpr std::size_t kMaxRandomnessResponseSize = 131072;

net::NetworkTrafficAnnotationTag RandomnessRequestAnnotation() {
  return net::DefineNetworkTrafficAnnotation("p3a_star_randomness", R"(
    semantics {
      sender: "Brave Privacy-Preserving Product Analytics STAR"
      description:
        "Requests randomness for a single analytics metric."
        "The randomness data is used to create a key for encrypting analytics data "
        "using the STAR protocol, to protect user anonymity."
        "See https://arxiv.org/abs/2109.10074 for more information."
      trigger:
        "Requests are automatically sent at intervals "
        "while Brave is running."
      data: "Anonymous usage data."
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      setting:
        "Users can enable or disable it in brave://settings/privacy"
       policy_exception_justification:
         "Not implemented."
    })");
}

}  // namespace

BraveP3AStarManager::BraveP3AStarManager(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    StarMessageCallback message_callback,
    const GURL& randomness_server_url,
    bool use_local_randomness)
    : current_public_key_(nested_star::get_ppoprf_null_public_key()),
      url_loader_factory_(url_loader_factory),
      message_callback_(message_callback),
      randomness_server_url_(randomness_server_url),
      use_local_randomness_(use_local_randomness) {}

BraveP3AStarManager::~BraveP3AStarManager() {}

bool BraveP3AStarManager::StartMessagePreparation(const char* histogram_name,
                                                  uint8_t epoch,
                                                  std::string serialized_log) {
  std::vector<std::string> layers =
      base::SplitString(serialized_log, kP3AMessageStarLayerSeparator,
                        base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);

  auto prepare_res = nested_star::prepare_measurement(layers, epoch);
  if (!prepare_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarManager: measurement preparation failed: "
               << prepare_res.error.c_str();
    return false;
  }

  auto req_res = nested_star::construct_randomness_request(*prepare_res.state);
  if (!req_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarManager: randomness req preparation failed: "
               << req_res.error.c_str();
    return false;
  }

  if (use_local_randomness_) {
    // For dev/test purposes only
    auto local_rand_res = nested_star::generate_local_randomness(req_res.data);
    if (!local_rand_res.error.empty()) {
      LOG(ERROR) << "BraveP3AStarManager: generating local randomness failed: "
                 << local_rand_res.error.c_str();
      return false;
    }

    std::string local_rand_data(local_rand_res.data);

    std::string msg_output;
    if (!ConstructFinalMessage(prepare_res.state, local_rand_data,
                               &msg_output)) {
      return false;
    }

    message_callback_.Run(
        histogram_name, epoch,
        std::unique_ptr<std::string>(new std::string(msg_output)));
  } else {
    SendRandomnessRequest(histogram_name, epoch, std::move(prepare_res.state),
                          std::string(req_res.data));
  }

  return true;
}

void BraveP3AStarManager::SendRandomnessRequest(
    const char* histogram_name,
    uint8_t epoch,
    rust::Box<nested_star::RandomnessRequestStateWrapper>
        randomness_request_state,
    std::string rand_req_data) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = randomness_server_url_;
  resource_request->method = "POST";

  url_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 RandomnessRequestAnnotation());

  url_loader_->AttachStringForUpload(rand_req_data, "application/json");

  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP3AStarManager::HandleRandomnessResponse,
                     base::Unretained(this), histogram_name, epoch,
                     std::move(randomness_request_state)),
      kMaxRandomnessResponseSize);
}

void BraveP3AStarManager::HandleRandomnessResponse(
    const char* histogram_name,
    uint8_t epoch,
    rust::Box<nested_star::RandomnessRequestStateWrapper>
        randomness_request_state,
    std::unique_ptr<std::string> response_body) {
  url_loader_.reset();
  if (!response_body || response_body->empty()) {
    LOG(ERROR)
        << "BraveP3AStarManager: no response body from randomness server";
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }
  std::string final_msg;
  if (!ConstructFinalMessage(randomness_request_state, *response_body,
                             &final_msg)) {
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }

  message_callback_.Run(
      histogram_name, epoch,
      std::unique_ptr<std::string>(new std::string(final_msg)));
}

bool BraveP3AStarManager::ConstructFinalMessage(
    rust::Box<nested_star::RandomnessRequestStateWrapper>&
        randomness_request_state,
    const std::string& rand_resp_data,
    std::string* output) {
  auto msg_res = nested_star::construct_message(
      rand_resp_data, *randomness_request_state, *current_public_key_, {},
      kP3AStarCurrentThreshold);
  if (!msg_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarManager: message construction failed: "
               << msg_res.error.c_str();
    return false;
  }

  *output = base::Base64Encode(msg_res.data);
  return true;
}

}  // namespace brave

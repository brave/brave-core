/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/is_request_body.h"
#include "brave/components/brave_account/endpoint_client/retry_options.h"
#include "brave/components/brave_account/endpoint_client/url_replacements.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace brave_account::endpoint_client::detail {

// HTTP methods
enum class Method {
  kConnect,
  kDelete,
  kGet,
  kHead,
  kOptions,
  kPatch,
  kPost,
  kPut,
  kTrace,
  kTrack
};

// A request binds a request body type to a specific HTTP method and
// provides access to the method, Content-Type, and serialized body.
template <IsRequestBody RequestBody, Method M>
struct Request {
  using Body = RequestBody;

  static constexpr std::string_view Method() {
    if constexpr (M == Method::kConnect) {
      return net::HttpRequestHeaders::kConnectMethod;
    } else if constexpr (M == Method::kDelete) {
      return net::HttpRequestHeaders::kDeleteMethod;
    } else if constexpr (M == Method::kGet) {
      return net::HttpRequestHeaders::kGetMethod;
    } else if constexpr (M == Method::kHead) {
      return net::HttpRequestHeaders::kHeadMethod;
    } else if constexpr (M == Method::kOptions) {
      return net::HttpRequestHeaders::kOptionsMethod;
    } else if constexpr (M == Method::kPatch) {
      return net::HttpRequestHeaders::kPatchMethod;
    } else if constexpr (M == Method::kPost) {
      return net::HttpRequestHeaders::kPostMethod;
    } else if constexpr (M == Method::kPut) {
      return net::HttpRequestHeaders::kPutMethod;
    } else if constexpr (M == Method::kTrace) {
      return net::HttpRequestHeaders::kTraceMethod;
    } else if constexpr (M == Method::kTrack) {
      return net::HttpRequestHeaders::kTrackMethod;
    } else {
      static_assert(false, "Unhandled Method enumerator!");
    }
  }

  static constexpr std::string_view ContentType() {
    return ContentType<Body>();
  }

  std::optional<std::string> Serialize() const { return Serialize(body); }

  Body body;
  net::MutableNetworkTrafficAnnotationTag network_traffic_annotation_tag;
  RetryOptions retry_options;
  base::TimeDelta timeout_duration;
  UrlReplacements url_replacements;

 private:
  // Returns the Content-Type value for a JSON request body.
  template <IsJSONRequestBody>
  static constexpr std::string_view ContentType() {
    return "application/json";
  }

  // Returns the Content-Type value for a Protobuf request body.
  template <IsProtobufRequestBody>
  static constexpr std::string_view ContentType() {
    return "application/protobuf";
  }

  // Serializes a JSON request body to a JSON string.
  // Returns std::nullopt if the request body produces an empty dictionary.
  static std::optional<std::string> Serialize(
      const IsJSONRequestBody auto& request_body) {
    const base::DictValue dict = request_body.ToValue();
    if (dict.empty()) {
      return std::nullopt;
    }

    return CHECK_DEREF(base::WriteJson(dict));
  }

  // Serializes a Protobuf request body to a binary string.
  // Returns std::nullopt if the request body is empty (ByteSizeLong() == 0).
  static std::optional<std::string> Serialize(
      const IsProtobufRequestBody auto& request_body) {
    // IsInitialized() is false only if a proto2 required field is unset, so
    // this is a no-op for proto3. Protobuf itself merely DCHECKs it, so release
    // builds would otherwise silently send an incomplete message. It must run
    // before the empty check: unset required fields add nothing to
    // ByteSizeLong(), so such a message would otherwise be sent with no body.
    CHECK(request_body.IsInitialized());

    if (!request_body.ByteSizeLong()) {
      return std::nullopt;
    }

    std::string serialized;
    CHECK(request_body.SerializeToString(&serialized));
    return serialized;
  }
};

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_H_

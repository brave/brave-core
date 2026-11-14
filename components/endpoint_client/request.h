/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_

#include <string>
#include <string_view>

#include "base/values.h"
#include "brave/components/endpoint_client/is_request_body.h"
#include "brave/components/endpoint_client/request_options.h"
#include "net/http/http_request_headers.h"

namespace google::protobuf {
class MessageLite;
}  // namespace google::protobuf

namespace endpoint_client::internal {

std::string SerializeJSONRequestBody(const base::DictValue& request_body);

std::string SerializeProtobufRequestBody(
    const google::protobuf::MessageLite& request_body);

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

constexpr bool MethodAllowsBody(Method method) {
  return method != Method::kGet && method != Method::kHead;
}

template <Method M, typename...>
struct Request {
  static_assert(false, "Request<> takes at most one body type!");
};

// A body-less request has a fixed HTTP method and provides access to it.
template <Method M>
struct Request<M> : RequestOptions {
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
};

template <Method M, typename T>
struct Request<M, T> {
  static_assert(false,
                "MethodAllowsBody(M) must be true, and T must satisfy "
                "IsRequestBody!");
};

// A request with a body extends the body-less request with a request body
// type, and provides access to the Content-Type and serialized body.
template <Method M, IsRequestBody RequestBody>
  requires(MethodAllowsBody(M))
struct Request<M, RequestBody> : Request<M> {
  using Body = RequestBody;

  static constexpr std::string_view ContentType() {
    return ContentType<Body>();
  }

  std::string Serialize() const { return Serialize(body); }

  Body body;

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
  static std::string Serialize(const IsJSONRequestBody auto& request_body) {
    return SerializeJSONRequestBody(request_body.ToValue());
  }

  // Serializes a Protobuf request body to a binary string.
  static std::string Serialize(const IsProtobufRequestBody auto& request_body) {
    return SerializeProtobufRequestBody(request_body);
  }
};

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_

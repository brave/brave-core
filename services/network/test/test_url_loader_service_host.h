/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_SERVICES_NETWORK_TEST_TEST_URL_LOADER_SERVICE_HOST_H_
#define BRAVE_SERVICES_NETWORK_TEST_TEST_URL_LOADER_SERVICE_HOST_H_

#include <cstdint>
#include <memory>

#include "brave/services/network/simple_url_loader_service.h"
#include "services/network/test/test_url_loader_factory.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave::network::test {

// Owns a fake network stack and a SimpleUrlLoaderService bound to it, so that
// a Rust test can drive the service over a real message pipe without touching
// the network.
//
// This exists because the pieces a Rust caller needs (a URLLoaderFactory, a
// traffic annotation, a sequence) are all C++ concepts. In production the
// equivalent setup would live in whichever browser-side class already owns a
// SharedURLLoaderFactory.
class TestUrlLoaderServiceHost {
 public:
  TestUrlLoaderServiceHost();

  TestUrlLoaderServiceHost(const TestUrlLoaderServiceHost&) = delete;
  TestUrlLoaderServiceHost& operator=(const TestUrlLoaderServiceHost&) = delete;

  ~TestUrlLoaderServiceHost();

  // Seeds a canned response. `response_code` is an HTTP status code.
  void AddResponse(rust::Str url, rust::Str content, int32_t response_code);

  // Total requests the fake factory has seen, so a test can assert that a
  // request really reached the network layer rather than being short-circuited.
  size_t TotalRequests() const;

  void Bind(mojo::PendingReceiver<mojom::SimpleUrlLoader> receiver);

 private:
  ::network::TestURLLoaderFactory url_loader_factory_;
  SimpleUrlLoaderService service_;
};

// Creates a host and hands back the client end of a pipe bound to its service.
// `out_handle` receives a raw, unowned message pipe handle which the caller
// takes ownership of.
std::unique_ptr<TestUrlLoaderServiceHost> CreateTestUrlLoaderServiceHost(
    uintptr_t& out_handle);

}  // namespace brave::network::test

#endif  // BRAVE_SERVICES_NETWORK_TEST_TEST_URL_LOADER_SERVICE_HOST_H_

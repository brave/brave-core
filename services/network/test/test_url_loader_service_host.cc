/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/services/network/test/test_url_loader_service_host.h"

#include <string>
#include <utility>

#include "base/task/sequenced_task_runner.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
// GetSafeWeakWrapper() returns a scoped_refptr to a type that
// test_url_loader_factory.h only forward declares, so the upcast to
// SharedURLLoaderFactory needs the full definition.
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"

namespace brave::network::test {

namespace {

// Traffic annotations must be compile-time constants so that
// traffic_annotation_auditor can find them, which is precisely why the Mojom
// interface does not let the caller supply one.
constexpr net::NetworkTrafficAnnotationTag kTestTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_rust_url_loader_test", R"(
      semantics {
        sender: "Brave Rust URL loader test"
        description: "Test-only traffic. Never reaches a real network."
        trigger: "Running brave_unit_tests."
        data: "None."
        destination: OTHER
      }
      policy {
        cookies_allowed: NO
        setting: "Not user visible; test-only."
        policy_exception_justification: "Test-only code."
      })");

}  // namespace

TestUrlLoaderServiceHost::TestUrlLoaderServiceHost()
    // JSON sanitization runs on the test's own sequence: the Rust tests use
    // base::test::SingleThreadTaskEnvironment, which is MAIN_THREAD_ONLY and so
    // has no thread pool to post to.
    : service_(url_loader_factory_.GetSafeWeakWrapper(),
               kTestTrafficAnnotation,
               base::SequencedTaskRunner::GetCurrentDefault()) {}

TestUrlLoaderServiceHost::~TestUrlLoaderServiceHost() = default;

void TestUrlLoaderServiceHost::AddResponse(rust::Str url,
                                           rust::Str content,
                                           int32_t response_code) {
  url_loader_factory_.AddResponse(
      std::string(url), std::string(content),
      static_cast<net::HttpStatusCode>(response_code));
}

size_t TestUrlLoaderServiceHost::TotalRequests() const {
  return url_loader_factory_.total_requests();
}

void TestUrlLoaderServiceHost::Bind(
    mojo::PendingReceiver<mojom::SimpleUrlLoader> receiver) {
  service_.Bind(std::move(receiver));
}

std::unique_ptr<TestUrlLoaderServiceHost> CreateTestUrlLoaderServiceHost(
    uintptr_t& out_handle) {
  auto host = std::make_unique<TestUrlLoaderServiceHost>();
  mojo::PendingRemote<mojom::SimpleUrlLoader> remote;
  host->Bind(remote.InitWithNewPipeAndPassReceiver());
  out_handle = remote.PassPipe().release().value();
  return host;
}

}  // namespace brave::network::test

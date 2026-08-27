/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"
#include "net/cert/caching_cert_verifier.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/coalescing_cert_verifier.h"
#include "net/cert_net/cert_net_fetcher_url_request.h"
#include "net/net_buildflags.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

// Per transport_security_state_static.h:
//
// Note that the consumer must include
// "net/http/transport_security_state_source.h", as this file cannot include
// any headers itself, since it's always included in a nested namespace.
#include "net/http/transport_security_state_source.h"  // IWYU pragma: keep

#if BUILDFLAG(INCLUDE_TRANSPORT_SECURITY_STATE_PRELOAD_LIST)
namespace net {
namespace {
#include "net/http/transport_security_state_static.h"
}  // namespace
}  // namespace net
#endif

namespace brave {
namespace {

class BraveCertPinningTest : public testing::TestWithParam<std::string_view> {
 protected:
  BraveCertPinningTest()
      : task_environment_(base::test::TaskEnvironment::MainThreadType::IO) {}

  void SetUp() override {
    // Build a minimal context for the fetcher's own requests (AIA, OCSP, CRL).
    net::URLRequestContextBuilder fetcher_builder;
    fetcher_context_ = fetcher_builder.Build();

    // Create the fetcher and wire it to the fetcher context.
    //
    // CertNetFetcher is required for hosts which might serve incomplete CA
    // chains. Absent a CertNetFetcher, the Cert will be considered invalid,
    // regardless of pinning.
    cert_net_fetcher_ = base::MakeRefCounted<net::CertNetFetcherURLRequest>();
    cert_net_fetcher_->SetURLRequestContext(fetcher_context_.get());

    // Build the main context with a wrapped CertVerifier that uses the fetcher.
    auto base_verifier =
        net::CertVerifier::CreateDefaultWithoutCaching(cert_net_fetcher_);
    net::URLRequestContextBuilder builder;
    builder.SetCertVerifier(std::make_unique<net::CachingCertVerifier>(
        std::make_unique<net::CoalescingCertVerifier>(
            std::move(base_verifier))));
    context_ = builder.Build();
  }

  // Performs a single GET request to the host and checks if the pinning
  // result (success or failure) matches the expectation.
  bool CheckHostOnce(const std::string& host,
                     bool expect_pin_failure,
                     std::string* error_out) {
    DCHECK(error_out);
    net::TestDelegate delegate;
    GURL url("https://" + host + "/");
    auto request =
        context_->CreateRequest(url, net::DEFAULT_PRIORITY, &delegate);
    request->Start();
    delegate.RunUntilComplete();

    // Certificate errors surface via OnSSLCertificateError() during the TLS
    // handshake. TestDelegate records the net error there and then cancels the
    // request, and URLRequest::Cancel() overwrites response_info_.ssl_info with
    // a default-constructed SSLInfo -- so request->ssl_info().cert_status is
    // always 0 by the time the run loop finishes. certificate_net_error() is
    // captured before the cancel and is the only surviving signal.
    int status = delegate.request_status();
    int cert_net_error = delegate.certificate_net_error();
    bool is_pin_failure =
        cert_net_error == net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN;

    VLOG(1) << host << ": status=" << net::ErrorToShortString(status)
            << " code=" << delegate.response_code().value_or(0)
            << " cert_net_error=" << net::ErrorToShortString(cert_net_error)
            << (is_pin_failure ? " [pin violated]" : "")
            << (expect_pin_failure ? " (expected pin failure)" : "");

    if (expect_pin_failure) {
      if (is_pin_failure) {
        return true;
      }
      *error_out = absl::StrFormat(
          "Expected pinning failure, got status=%s, code=%d, cert_net_error=%s",
          net::ErrorToShortString(status), delegate.response_code().value_or(0),
          net::ErrorToShortString(cert_net_error));
      return false;
    }

    // A pinned host is only healthy if the request completed cleanly. Merely
    // not tripping the pin check is not enough -- a DNS failure or an unrelated
    // certificate error would otherwise be reported as a pass.
    if (status == net::OK) {
      return true;
    }
    *error_out =
        absl::StrFormat("Expected success, got status=%s, cert_net_error=%s",
                        net::ErrorToShortString(status),
                        net::ErrorToShortString(cert_net_error));
    return false;
  }

  // Retries the pinning check per the backoff schedule: 0s, 0s, 2s.
  // Returns true if any attempt matched the expectation.
  bool CheckHostWithRetry(const std::string& host,
                          bool expect_pin_failure,
                          std::string* error_out) {
    DCHECK(error_out);
    const int max_attempts = 3;
    // Delays (in seconds) before each attempt: no delay for attempts 1&2, then
    // 2s.
    const std::array<int, 3> delay_seconds_cfg = {0, 0, 2};
    static_assert(delay_seconds_cfg.size() == max_attempts);

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
      auto delay_seconds = delay_seconds_cfg[attempt];
      if (delay_seconds > 0) {
        base::PlatformThread::Sleep(base::Seconds(delay_seconds));
      }

      if (CheckHostOnce(host, expect_pin_failure, error_out)) {
        return true;  // Success on this attempt.
      }

      LOG(WARNING) << "Attempt " << (attempt + 1) << " for " << host
                   << " failed: " << *error_out;
    }
    return false;  // All attempts failed.
  }

  void TearDown() override {
    // Must Shutdown() the fetcher while context_ is still valid, and before
    // context_ is destroyed. CertNetFetcherURLRequest's destructor DCHECKs
    // that Shutdown() already ran.
    cert_net_fetcher_->Shutdown();
  }

  base::test::TaskEnvironment task_environment_;
  scoped_refptr<net::CertNetFetcherURLRequest> cert_net_fetcher_;
  std::unique_ptr<net::URLRequestContext> fetcher_context_;
  std::unique_ptr<net::URLRequestContext> context_;
};

#if BUILDFLAG(INCLUDE_TRANSPORT_SECURITY_STATE_PRELOAD_LIST)

// Pinned hosts that cannot be checked from a developer or CI machine. Each
// entry needs a dated reason so stale skips can be pruned.
constexpr auto kSkippedHosts = std::to_array<std::string_view>({
    // 2026-08-28: The hostnames below do not resolve
    "api.gate3.brave.com",
    "fg.search.brave.com",
    "gaia.brave.com",
    "goerli-infura.brave.com",
    "innet-beta-solana.brave.com",
    "mainnet-infura.brave.com",
    "mainnet-beta-solana.brave.com",
    "mainnet-polygon.brave.com",
    "search.anonymous.brave.com",
    "search.anonymous.bravesoftware.com",
    "sepolia-infura.brave.com",
    "translate-static.brave.com",
    "wallet.brave.com",
});

// Test page served by a CA outside the pinset, so it is expected to fail
// pinning. This is the only host that proves the pin check can actually reject.
constexpr std::string_view kUnpinnedTestHost = "ssl-pinning.someblog.org";

std::vector<std::string_view> GetPinnedHosts() {
  std::vector<std::string_view> hosts;
  hosts.reserve(net::kHostPins.size());
  for (const auto& [host, pin] : net::kHostPins) {
    hosts.push_back(host);
  }
  return hosts;
}

// gtest test names may only contain [A-Za-z0-9_], and hostnames are full of
// dots and dashes.
std::string HostToTestName(
    const testing::TestParamInfo<std::string_view>& info) {
  std::string name(info.param);
  std::ranges::replace_if(
      name,
      [](char c) {
        return !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
               !(c >= '0' && c <= '9');
      },
      '_');
  return name;
}

TEST_P(BraveCertPinningTest, PinValidates) {
  const std::string host(GetParam());

  if (std::ranges::find(kSkippedHosts, GetParam()) != kSkippedHosts.end()) {
    GTEST_SKIP() << "Host is in kSkippedHosts";
  }

  std::string error;
  EXPECT_TRUE(CheckHostWithRetry(host, GetParam() == kUnpinnedTestHost, &error))
      << host << ": " << error;
}

INSTANTIATE_TEST_SUITE_P(All,
                         BraveCertPinningTest,
                         testing::ValuesIn(GetPinnedHosts()),
                         HostToTestName);

#endif  // BUILDFLAG(INCLUDE_TRANSPORT_SECURITY_STATE_PRELOAD_LIST)

}  // namespace
}  // namespace brave

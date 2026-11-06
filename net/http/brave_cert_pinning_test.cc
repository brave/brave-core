/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/location.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/task_environment.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"
#include "net/cert/caching_cert_verifier.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/coalescing_cert_verifier.h"
#include "net/cert_net/cert_net_fetcher_url_request.h"
#include "net/http/transport_security_state.h"
#include "net/net_buildflags.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// Per transport_security_state_static.h:
//
// Note that the consumer must include
// "net/http/transport_security_state_source.h", as this file cannot include
// any headers itself, since it's always included in a nested namespace.
#include "net/http/transport_security_state_source.h"  // IWYU pragma: keep

// Only fails on ChromeOS, which we don't build. Just assert.
static_assert(BUILDFLAG(INCLUDE_TRANSPORT_SECURITY_STATE_PRELOAD_LIST));

namespace net {
namespace {
#include "net/http/transport_security_state_static_pins.h"
// Pins header must be included first because this header uses kFindHostPin
// defined in the pins header.
#include "net/http/transport_security_state_static.h"
}  // namespace
}  // namespace net

namespace brave {

namespace {

enum class Verdict {
  // Pinning behaved as expected for the host.
  kAsExpected,
  // The request reached a verdict and it was not the expected one. This
  // includes every certificate error, so a pin violation is never downgraded.
  kUnexpected,
  // The request timed out without completing or reporting a certificate
  // error, so the pin check was never shown to fail.
  kInconclusive,
};

struct CheckResult {
  Verdict verdict;
  std::string message;
};

}  // namespace

class BraveCertPinningTest : public testing::TestWithParam<std::string_view> {
 protected:
  BraveCertPinningTest()
      : task_environment_(base::test::TaskEnvironment::MainThreadType::IO) {}

  void SetUp() override {
    // Build a minimal context for the fetcher's own requests (AIA, OCSP, CRL).
    net::URLRequestContextBuilder fetcher_builder;
    SetDirectProxyConfig(fetcher_builder);
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
    SetDirectProxyConfig(builder);
    context_ = builder.Build();

#if BUILDFLAG(IS_IOS)
    // Without this, iOS (and only iOS) appears to bypass pin violations for
    // this test's CA, since CertVerifyProcIOS can't reliably determine
    // is_issued_by_known_root the way desktop's builtin verifier can.
    context_->transport_security_state()
        ->SetEnablePublicKeyPinningBypassForLocalTrustAnchors(false);
#endif  // BUILDFLAG(IS_IOS)
  }

  // On Linux and Android, URLRequestContextBuilder does not create a default
  // system ProxyConfigService (see url_request_context_builder.cc), so Build()
  // DCHECKs unless one is supplied. Windows/Mac already get a real system proxy
  // config service from the builder, so leave those alone.
  static void SetDirectProxyConfig(net::URLRequestContextBuilder& builder) {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_ANDROID)
    builder.set_proxy_config_service(
        std::make_unique<net::ProxyConfigServiceFixed>(
            net::ProxyConfigWithAnnotation::CreateDirect()));
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_ANDROID)
  }

  // Performs a single GET request to the host and checks if the pinning
  // result (success or failure) matches the expectation.
  CheckResult CheckHostOnce(const std::string& host, bool expect_pin_failure) {
    net::TestDelegate delegate;
    GURL url(base::StrCat(
        {url::kHttpsScheme, url::kStandardSchemeSeparator, host, "/"}));
    auto request = context_->CreateRequest(
        url, net::DEFAULT_PRIORITY, &delegate, TRAFFIC_ANNOTATION_FOR_TESTS,
        net::handles::kInvalidNetworkHandle);

    // Bound the wait ourselves. The RunLoop timeout that TaskEnvironment
    // installs records a test failure when it fires, which neither the retries
    // nor the inconclusive verdict can undo, so it is disabled for this wait.
    // The deadline keeps the budget it had: 3 attempts take at most
    // 3 * action_timeout() + 2s, under the test launcher's 45s default.
    base::RunLoop run_loop;
    delegate.set_on_complete(run_loop.QuitClosure());
    base::OneShotTimer deadline;
    deadline.Start(FROM_HERE, TestTimeouts::action_timeout(),
                   run_loop.QuitClosure());
    {
      base::test::ScopedDisableRunLoopTimeout disable_run_loop_timeout;
      request->Start();
      run_loop.Run();
    }

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
    // The deadline fired before the request completed or reported a
    // certificate error. Any certificate error, pin violations included, is a
    // verdict and is never treated as a timeout.
    bool timed_out =
        !delegate.response_completed() && !delegate.have_certificate_errors();
    // OnConnected() only runs once the TLS handshake, which includes the pin
    // check, has succeeded.
    bool handshake_succeeded = !delegate.transports().empty();

    VLOG(1) << host << ": status=" << net::ErrorToShortString(status)
            << " code=" << delegate.response_code().value_or(0)
            << " cert_net_error=" << net::ErrorToShortString(cert_net_error)
            << (is_pin_failure ? " [pin violated]" : "")
            << (timed_out ? " [timed out]" : "")
            << (handshake_succeeded ? " [handshake succeeded]" : "")
            << (expect_pin_failure ? " (expected pin failure)" : "");

    std::string failure;
    if (expect_pin_failure) {
      if (is_pin_failure) {
        return {Verdict::kAsExpected, ""};
      }
      failure = base::StrCat(
          {"Expected pinning failure, got status=",
           net::ErrorToShortString(status), ", code=",
           base::NumberToString(delegate.response_code().value_or(0)),
           ", cert_net_error=", net::ErrorToShortString(cert_net_error)});
    } else {
      // A pinned host is only healthy if the request completed cleanly. Merely
      // not tripping the pin check is not enough -- a DNS failure or an
      // unrelated certificate error would otherwise be reported as a pass.
      if (status == net::OK && !timed_out) {
        return {Verdict::kAsExpected, ""};
      }
      failure = base::StrCat(
          {"Expected success, got status=", net::ErrorToShortString(status),
           ", cert_net_error=", net::ErrorToShortString(cert_net_error)});
    }
    if (timed_out) {
      failure = base::StrCat({failure, " (timed out, handshake succeeded: ",
                              handshake_succeeded ? "yes" : "no", ")"});
      // No verdict, unless the host that must fail pinning completed its
      // handshake: the pin check then accepted the connection.
      if (!(expect_pin_failure && handshake_succeeded)) {
        return {Verdict::kInconclusive, failure};
      }
    }
    return {Verdict::kUnexpected, failure};
  }

  // Retries the pinning check per the backoff schedule: 0s, 0s, 2s.
  // Returns as soon as an attempt matches the expectation. Otherwise returns
  // the last attempt that reached a verdict, falling back to the last
  // inconclusive attempt only if every attempt was, so a timeout never hides a
  // verdict.
  CheckResult CheckHostWithRetry(const std::string& host,
                                 bool expect_pin_failure) {
    const int max_attempts = 3;
    // Delays (in seconds) before each attempt: no delay for attempts 1&2, then
    // 2s.
    const std::array<int, 3> delay_seconds_cfg = {0, 0, 2};
    static_assert(delay_seconds_cfg.size() == max_attempts);

    std::optional<CheckResult> last_unexpected;
    std::optional<CheckResult> last_inconclusive;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
      auto delay_seconds = delay_seconds_cfg[attempt];
      if (delay_seconds > 0) {
        base::PlatformThread::Sleep(base::Seconds(delay_seconds));
      }

      CheckResult result = CheckHostOnce(host, expect_pin_failure);
      if (result.verdict == Verdict::kAsExpected) {
        return result;  // Success on this attempt.
      }

      const bool inconclusive = result.verdict == Verdict::kInconclusive;
      LOG(WARNING) << "Attempt " << (attempt + 1) << " for " << host
                   << (inconclusive ? " inconclusive: " : " failed: ")
                   << result.message;
      (inconclusive ? last_inconclusive : last_unexpected) = std::move(result);
    }
    return last_unexpected ? *std::move(last_unexpected)
                           : *std::move(last_inconclusive);
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

  const CheckResult result =
      CheckHostWithRetry(host, GetParam() == kUnpinnedTestHost);
  if (result.verdict == Verdict::kInconclusive) {
    GTEST_SKIP() << host << ": " << result.message;
  }
  EXPECT_TRUE(result.verdict == Verdict::kAsExpected)
      << host << ": " << result.message;
}

INSTANTIATE_TEST_SUITE_P(All,
                         BraveCertPinningTest,
                         testing::ValuesIn(GetPinnedHosts()),
                         HostToTestName);

}  // namespace brave

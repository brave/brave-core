/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Based on chromium's
// chrome/browser/ssl/certificate_transparency_browsertest.cc under this
// license:
//
//   Copyright 2022 The Chromium Authors
//   Use of this source code is governed by a BSD-style license that can be
//   found in the LICENSE file.

#include <string_view>

#include "base/run_loop.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ssl/ssl_browsertest_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test.h"
#include "crypto/sha2.h"
#include "net/base/hash_value.h"
#include "net/cert/asn1_util.h"
#include "net/cert/x509_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/cert_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/test_data_directory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Returns the Sha256 hash of the SPKI of |cert|.
net::HashValue GetSPKIHash(const CRYPTO_BUFFER* cert) {
  std::string_view spki_bytes;
  EXPECT_TRUE(net::asn1::ExtractSPKIFromDERCert(
      net::x509_util::CryptoBufferAsStringPiece(cert), &spki_bytes));
  net::HashValue sha256(net::HASH_VALUE_SHA256);
  crypto::SHA256HashString(spki_bytes, sha256.data(), crypto::kSHA256Length);
  return sha256;
}

}  // namespace

// Class used to run browser tests that verify SSL UI triggered due to
// Certificate Transparency verification failures/successes.
class CertificateTransparencyBrowserTest : public CertVerifierBrowserTest {
 public:
  CertificateTransparencyBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    SystemNetworkContextManager::SetEnableCertificateTransparencyForTesting(
        true);
  }

  CertificateTransparencyBrowserTest(
      const CertificateTransparencyBrowserTest&) = delete;
  CertificateTransparencyBrowserTest& operator=(
      const CertificateTransparencyBrowserTest&) = delete;

  ~CertificateTransparencyBrowserTest() override {}

  void SetUpOnMainThread() override {
    CertVerifierBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
  }

  void SetUp() override {
    policy_provider_.SetDefaultReturns(
        /*is_initialization_complete_return=*/true,
        /*is_first_policy_load_complete_return=*/true);
    CertVerifierBrowserTest::SetUp();
  }

  void SetUpCertVerifier() {
    SystemNetworkContextManager::GetInstance()->SetCTLogListTimelyForTesting();

    ASSERT_TRUE(https_server()->Start());

    net::CertVerifyResult verify_result;
    verify_result.verified_cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), "may_2018.pem");
    ASSERT_TRUE(verify_result.verified_cert);
    verify_result.is_issued_by_known_root = true;
    verify_result.public_key_hashes.push_back(
        GetSPKIHash(verify_result.verified_cert->cert_buffer()));

    mock_cert_verifier()->AddResultForCert(
        https_server()->GetCertificate().get(), verify_result, net::OK);
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

 private:
  net::EmbeddedTestServer https_server_;

  testing::NiceMock<policy::MockConfigurationPolicyProvider> policy_provider_;
};

IN_PROC_BROWSER_TEST_F(CertificateTransparencyBrowserTest, EnforcedByDefault) {
  SetUpCertVerifier();

  // Normal non-exempt URL
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("/ssl/google.html")));

  ssl_test_util::CheckSecurityState(
      browser()->tab_strip_model()->GetActiveWebContents(),
      net::CERT_STATUS_CERTIFICATE_TRANSPARENCY_REQUIRED,
      security_state::DANGEROUS,
      ssl_test_util::AuthState::SHOWING_INTERSTITIAL);
}

IN_PROC_BROWSER_TEST_F(CertificateTransparencyBrowserTest, ExemptedHost) {
  SetUpCertVerifier();

  // URL exempted from SCT requirements
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("sct-exempted.bravesoftware.com",
                                        "/ssl/google.html")));

  ssl_test_util::CheckSecurityState(
      browser()->tab_strip_model()->GetActiveWebContents(),
      ssl_test_util::CertError::NONE, security_state::SECURE,
      ssl_test_util::AuthState::NONE);
}

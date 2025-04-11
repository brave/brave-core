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

#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/ssl/ssl_browsertest_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/cert_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

// Class used to run browser tests that verify SSL UI triggered due to
// Certificate Transparency verification failures.
class CertificateTransparencyBrowserTest : public InProcessBrowserTest {
 public:
  CertificateTransparencyBrowserTest() {
    SystemNetworkContextManager::SetEnableCertificateTransparencyForTesting(
        true);
  }

  CertificateTransparencyBrowserTest(
      const CertificateTransparencyBrowserTest&) = delete;
  CertificateTransparencyBrowserTest& operator=(
      const CertificateTransparencyBrowserTest&) = delete;

  ~CertificateTransparencyBrowserTest() override {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_https_test_server().SetCertHostnames(
        {"sct-exempted.bravesoftware.com"});
    ASSERT_TRUE(embedded_https_test_server().Start());
  }
};

IN_PROC_BROWSER_TEST_F(CertificateTransparencyBrowserTest, EnforcedByDefault) {
  SystemNetworkContextManager::GetInstance()->SetCTLogListTimelyForTesting();

  // Make the test root be interpreted as a known root so that CT will be
  // required.
  scoped_refptr<net::X509Certificate> root_cert =
      net::ImportCertFromFile(net::EmbeddedTestServer::GetRootCertPemPath());
  ASSERT_TRUE(root_cert);
  net::ScopedTestKnownRoot scoped_known_root(root_cert.get());

  // Normal non-exempt URL
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      embedded_https_test_server().GetURL("example.com", "/ssl/google.html")));

  ssl_test_util::CheckSecurityState(
      browser()->tab_strip_model()->GetActiveWebContents(),
      net::CERT_STATUS_CERTIFICATE_TRANSPARENCY_REQUIRED,
      security_state::DANGEROUS,
      ssl_test_util::AuthState::SHOWING_INTERSTITIAL);
}

IN_PROC_BROWSER_TEST_F(CertificateTransparencyBrowserTest, ExemptedHost) {
  SystemNetworkContextManager::GetInstance()->SetCTLogListTimelyForTesting();

  // Make the test root be interpreted as a known root so that CT will be
  // required.
  scoped_refptr<net::X509Certificate> root_cert =
      net::ImportCertFromFile(net::EmbeddedTestServer::GetRootCertPemPath());
  ASSERT_TRUE(root_cert);
  net::ScopedTestKnownRoot scoped_known_root(root_cert.get());

  // URL exempted from SCT requirements
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_https_test_server().GetURL(
                     "sct-exempted.bravesoftware.com", "/ssl/google.html")));

  ssl_test_util::CheckSecurityState(
      browser()->tab_strip_model()->GetActiveWebContents(),
      ssl_test_util::CertError::NONE, security_state::SECURE,
      ssl_test_util::AuthState::NONE);
}

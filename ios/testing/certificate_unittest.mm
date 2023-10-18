/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/net/certificate_utility.h"
#import "testing/gtest/include/gtest/gtest.h"
#import "testing/platform_test.h"
#include "net/http/transport_security_state.h"
#include "net/net_buildflags.h"

#import <Security/Security.h>

using CertificateTest = PlatformTest;

namespace net {
namespace {
#if BUILDFLAG(INCLUDE_TRANSPORT_SECURITY_STATE_PRELOAD_LIST)
#include "net/http/transport_security_state_static.h"  // nogncheck
#endif  // INCLUDE_TRANSPORT_SECURITY_STATE_PRELOAD_LIST
}  // namespace
}  // namespace net

namespace {

// AmazonRootCA1
// From:
// https://github.com/brave/brave-core/blob/master/chromium_src/net/tools/transport_security_state_generator/input_file_parsers.cc
static const char* brave_test_cert = R"(
    MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
    ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
    b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
    MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
    b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
    ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
    9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
    IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
    VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
    93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
    jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
    AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
    A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
    U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
    N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
    o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
    5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
    rqXRfboQnoZsG4q5WTP468SQvvG5
)";

SecCertificateRef GetTestCertificate() {
  NSString* cert_string = [NSString stringWithFormat:@"%s", brave_test_cert];
  cert_string = [cert_string stringByReplacingOccurrencesOfString:@"\t"
                                                       withString:@""];
  cert_string = [cert_string stringByReplacingOccurrencesOfString:@"\r"
                                                       withString:@""];
  cert_string = [cert_string stringByReplacingOccurrencesOfString:@"\n"
                                                       withString:@""];

  NSData* data = [[NSData alloc]
      initWithBase64EncodedString:cert_string
                          options:NSDataBase64DecodingIgnoreUnknownCharacters];
  if (!data) {
    return nil;
  }
  return SecCertificateCreateWithData(kCFAllocatorDefault, (__bridge CFDataRef)data);
}

}  // namespace

TEST_F(CertificateTest, LoadingCertificate) {
  EXPECT_NE(GetTestCertificate(), nil);
}

TEST_F(CertificateTest, AcceptableCertificates) {
  std::size_t acceptable_certs_count =
      sizeof(net::kBraveAcceptableCerts) /
          sizeof(net::kBraveAcceptableCerts[0]) -
      1;

  NSArray<NSData*>* certs = [BraveCertificateUtility acceptableSPKIHashes];

  EXPECT_TRUE([certs count] > 3);
  EXPECT_EQ([certs count], acceptable_certs_count);

  std::string test_cert = std::string(net::kBraveAcceptableCerts[0]);
  if (test_cert.size() > 0) {
    NSData* data = [NSData dataWithBytes:&test_cert[0] length:test_cert.size()];
    EXPECT_NE(data, nil);

    EXPECT_TRUE([certs[0] isEqual:data]);
  }
}

TEST_F(CertificateTest, HashCertificateSPKI) {
  std::string amazon_root_ca1_spki_hash =
      std::string(net::kSPKIHash_AmazonRootCA1);
  EXPECT_TRUE(amazon_root_ca1_spki_hash.size() > 0);

  NSData* amzn_spki_hash_data =
      [NSData dataWithBytes:&amazon_root_ca1_spki_hash[0]
                     length:amazon_root_ca1_spki_hash.size()];
  EXPECT_NE(amzn_spki_hash_data, nil);
  EXPECT_TRUE([amzn_spki_hash_data length] > 0);

  NSData* spki_hash =
      [BraveCertificateUtility hashCertificateSPKI:GetTestCertificate()];
  EXPECT_NE(spki_hash, nil);
  EXPECT_TRUE([spki_hash length] > 0);

  EXPECT_TRUE([spki_hash isEqual:amzn_spki_hash_data]);
}

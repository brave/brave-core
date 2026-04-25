// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/nitro_utils/attestation.h"

#include <vector>

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "net/cert/x509_certificate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace nitro_utils {

namespace {

// Trimmed-down cbor+base64 attestation document for testing
// python: base64.b64encode(cbor2.dumps(map_dict, canonical=True))
constexpr char kShortDocument[] =
    "pmVub25jZVShkGajO9w8jptiAlBSISId8fsW9WZkaWdlc3RmU0hBMzg0aW1vZHVs"
    "ZV9pZHgnaS0wYjQ2MzcxODcxOWFkYjI0YS1lbmMwMTg4OTc4NjRhYmFlNWQyaXRp"
    "bWVzdGFtcBsAAAGIyxyk5Gl1c2VyX2RhdGFYIhIgQwHTmm6+N7JzV75L/9jKxVJK"
    "msJMA18TN9CiINDyqXpqcHVibGljX2tlefY=";
constexpr char kExpectedNonce[] = "a19066a33bdc3c8e9b6202505221221df1fb16f5";

// Version of the attestation document with randomized user_data and nonce
constexpr char kBadDocument[] =
    "pmVub25jZVRG4z08zx2z0jdL5cLAV9tBGzBcOWZkaWdlc3RmU0hBMzg0aW1vZHVsZV9pZHgn"
    "aS0wYjQ2MzcxODcxOWFkYjI0YS1lbmMwMTg4OTc4NjRhYmFlNWQyaXRpbWVzdGFtcBsAAAGI"
    "yxyk5Gl1c2VyX2RhdGFYIgogsn3PzVQFXXUPf6W5qVRUi3bVGRJ2C6PRri1YOt5YwqlqcHVi"
    "bGljX2tlefY=";

// Version of the attestation document with older user_data scheme
constexpr char kOldDocument[] =
    "pmVub25jZVShkGajO9w8jptiAlBSISId8fsW9WZkaWdlc3RmU0hBMzg0aW1vZHVs"
    "ZV9pZHgnaS0wYjQ2MzcxODcxOWFkYjI0YS1lbmMwMTg4OTc4NjRhYmFlNWQyaXRp"
    "bWVzdGFtcBsAAAGIyxyk5Gl1c2VyX2RhdGFYT3NoYTI1NjpDAdOabr43snNXvkv/"
    "2MrFUkqawkwDXxM30KIg0PKpejtzaGEyNTY6AAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAABqcHVibGljX2tlefY=";

// Version of the attestation document with randomized older scheme
// user_data and randomized nonce
constexpr char kOldBadDocument[] =
    "pmVub25jZVSYQjMTB+E+8zgs690i18gcDtPF5WZkaWdlc3RmU0hBMzg0aW1vZHVs"
    "ZV9pZHgnaS0wYjQ2MzcxODcxOWFkYjI0YS1lbmMwMTg4OTc4NjRhYmFlNWQyaXRp"
    "bWVzdGFtcBsAAAGIyxyk5Gl1c2VyX2RhdGFYT3NoYTI1NjpZ1l7mBoYUN/QYaRUP"
    "4qHov+2saZzBKCCBT16rA6fp6DtzaGEyNTY6psMTTAVeLx8shS1642BWxoW1q/dV"
    "RMvN0XG/pFkT4cVqcHVibGljX2tlefY=";

// TLS certificate from the same deployment as kShortDocument
// Certificates can be downloaded with the browser or on the command line:
// openssl s_client -showcerts -servername $HOST -connect $HOST:443 < /dev/null
constexpr char kTLSCert[] =
    "MIIEbjCCA1agAwIBAgISA0mFFumurFjCqNLirk4Wm7d1MA0GCSqGSIb3DQEBCwUA"
    "MDIxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQswCQYDVQQD"
    "EwJSMzAeFw0yMzA2MDcxOTIyMzVaFw0yMzA5MDUxOTIyMzRaMCUxIzAhBgNVBAMT"
    "GnN0YXItcmFuZHNydi5ic2cuYnJhdmUuY29tMFkwEwYHKoZIzj0CAQYIKoZIzj0D"
    "AQcDQgAE1pQSA2iDztH1KCIsNV2lcgLaO4X0yyKWUK7pUHsblKbc9YDOmJTdY71C"
    "LpnUkCCdcVzt3DpQPAlSWKaDWcprF6OCAlQwggJQMA4GA1UdDwEB/wQEAwIHgDAd"
    "BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDAYDVR0TAQH/BAIwADAdBgNV"
    "HQ4EFgQUX4xa6dlCn+f0oyP3SX4mxviUiW0wHwYDVR0jBBgwFoAUFC6zF7dYVsuu"
    "UAlA5h+vnYsUwsYwVQYIKwYBBQUHAQEESTBHMCEGCCsGAQUFBzABhhVodHRwOi8v"
    "cjMuby5sZW5jci5vcmcwIgYIKwYBBQUHMAKGFmh0dHA6Ly9yMy5pLmxlbmNyLm9y"
    "Zy8wJQYDVR0RBB4wHIIac3Rhci1yYW5kc3J2LmJzZy5icmF2ZS5jb20wTAYDVR0g"
    "BEUwQzAIBgZngQwBAgEwNwYLKwYBBAGC3xMBAQEwKDAmBggrBgEFBQcCARYaaHR0"
    "cDovL2Nwcy5sZXRzZW5jcnlwdC5vcmcwggEDBgorBgEEAdZ5AgQCBIH0BIHxAO8A"
    "dgC3Pvsk35xNunXyOcW6WPRsXfxCz3qfNcSeHQmBJe20mQAAAYiXhoXoAAAEAwBH"
    "MEUCIQCbUhIcQeR5JV9WOz6QW16E8jEfgTXgL83zKHkZ6F2onQIgaYbBQoVjr/5O"
    "ykoma+PKcdBhuBEaUstwyGcFUP+J5coAdQCt9776fP8QyIudPZwePhhqtGcpXc+x"
    "DCTKhYY069yCigAAAYiXhoYaAAAEAwBGMEQCIBhLLoiK6sZqFauJt7Ox3TkAwk60"
    "nUyVJeZ5jlQFzyxAAiANHCJYdN4JOBBivDq8z2Al+/8wkDwqazDgtkKm23CQyjAN"
    "BgkqhkiG9w0BAQsFAAOCAQEAJ4JJReEW05kimalJVibvTxdU8yNuBmPM+1KVD++H"
    "3ho1dQVpMTMOROpyiBNLyiiiG1lSmWMzJ0gVWAJu1pi87L3Ki8qld+58XYWKVWBt"
    "c2n5BNU3hCH3emoXzRRRA2VwXgX3oJ5qYnRrhPR3TvIvvh4uvYvUoNpE2l5ibqxC"
    "L+QgFdJkBQLHGrsLnoxZxNXwhI3VUjmoY1SOSVj2CeDVNEzPOpYsVL+2dSWXgKYw"
    "Q3L/xtSul9+NNKyzQecTqL8oUxIufYiPIpFZV/KicbzdOc/4S7uXoG8K5nbCnTba"
    "Dua1VozCsqvNQppqo76rhWGtdecduwm7VCtm4XC66MFJIA==";

class NitroAttestationTest : public testing::Test {
 public:
  NitroAttestationTest() = default;

 protected:
  std::vector<uint8_t> FromBase64(const char* b64_string) {
    return *base::Base64Decode(b64_string);
  }
  std::vector<uint8_t> FromHex(const char* hex_string) {
    std::vector<uint8_t> bytes;
    base::HexStringToBytes(hex_string, &bytes);
    return bytes;
  }
};

}  // namespace

TEST_F(NitroAttestationTest, Nonce) {
  // Parse a test document to pass for comparison.
  auto document = FromBase64(kShortDocument);
  auto expected_nonce = FromHex(kExpectedNonce);
  auto bad_document = FromBase64(kBadDocument);

  // Does the test document verify against the expected nonce?
  EXPECT_TRUE(VerifyNonceForTesting(document, expected_nonce));

  // Does the invalid document fail to verify against the nonce?
  EXPECT_FALSE(VerifyNonceForTesting(bad_document, expected_nonce));
}

TEST_F(NitroAttestationTest, UserData) {
  // Parse a TLS cert whose fingerprint matches the test
  // attestation document.
  auto cert_bytes = FromBase64(kTLSCert);
  auto cert = net::X509Certificate::CreateFromBytes(cert_bytes);
  ASSERT_TRUE(cert);

  // Does the test document verify against the expected cert fingerprint?
  auto document = FromBase64(kShortDocument);
  EXPECT_TRUE(VerifyUserDataKeyForTesting(document, cert));

  // Does the invalid document fail to verify against the same cert?
  auto bad = FromBase64(kBadDocument);
  EXPECT_FALSE(VerifyUserDataKeyForTesting(bad, cert));
}

TEST_F(NitroAttestationTest, OldUserData) {
  // Parse a TLS cert whose fingerprint matches the test
  // attestation document.
  auto cert_bytes = FromBase64(kTLSCert);
  auto cert = net::X509Certificate::CreateFromBytes(cert_bytes);
  ASSERT_TRUE(cert);

  // Does the old scheme document verify against the expected
  // cert fingerprint?
  auto old = FromBase64(kOldDocument);
  EXPECT_TRUE(VerifyUserDataKeyForTesting(old, cert));

  // Does the invalid document fail to verify against the same cert?
  auto bad = FromBase64(kOldBadDocument);
  EXPECT_FALSE(VerifyUserDataKeyForTesting(bad, cert));
}

}  // namespace nitro_utils

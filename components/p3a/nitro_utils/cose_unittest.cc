// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/nitro_utils/cose.h"

#include <algorithm>

#include "base/base64.h"
#include "components/cbor/reader.h"
#include "components/cbor/writer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace nitro_utils {

namespace {

constexpr char kExampleDocument[] =
    "hEShATgioFkQ86lpbW9kdWxlX2lkeCdpLTA5MzY1MTJkNWFlNmVlOTFhLWVuYzAxODIzYmNkZT"
    "hlYjhhOGFmZGlnZXN0ZlNIQTM4NGl0aW1lc3RhbXAbAAABgjvOTrZkcGNyc7AAWDAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABWDAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACWDAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADWDAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEWDBaPlS9Cl9QsbXsZB4GOh8eflqtwTn0n9h4"
    "SvypCoN2fsE91ws/"
    "Lj9DuOIzVbqqkFMFWDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAGWDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAHWDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIWD"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAJWDAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAKWDAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAALWDAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMWDAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANWDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOWDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAPWDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAABrY2VydGlmaWNhdGVZAn4wggJ6MIICAaADAgECAhABgjvN6OuKig"
    "AAAABi4DTQMAoGCCqGSM49BAMDMIGOMQswCQYDVQQGEwJVUzETMBEGA1UECAwKV2FzaGluZ3Rv"
    "bjEQMA4GA1UEBwwHU2VhdHRsZTEPMA0GA1UECgwGQW1hem9uMQwwCgYDVQQLDANBV1MxOTA3Bg"
    "NVBAMMMGktMDkzNjUxMmQ1YWU2ZWU5MWEudXMtd2VzdC0yLmF3cy5uaXRyby1lbmNsYXZlczAe"
    "Fw0yMjA3MjYxODM5MDlaFw0yMjA3MjYyMTM5MTJaMIGTMQswCQYDVQQGEwJVUzETMBEGA1UECA"
    "wKV2FzaGluZ3RvbjEQMA4GA1UEBwwHU2VhdHRsZTEPMA0GA1UECgwGQW1hem9uMQwwCgYDVQQL"
    "DANBV1MxPjA8BgNVBAMMNWktMDkzNjUxMmQ1YWU2ZWU5MWEtZW5jMDE4MjNiY2RlOGViOGE4YS"
    "51cy13ZXN0LTIuYXdzMHYwEAYHKoZIzj0CAQYFK4EEACIDYgAElCmQ1gW7KdxMCOgWtfANdRj3"
    "VECjJjp4ZdSjyI+tOsY3xSuEGjLyeSTZOuYNL1UqHx0H4GrD2S9d3XCGdWbzW4nA06vk6rRI/"
    "J8BNOdDutL/"
    "xUmxiBOWMWGSyQLitM5Sox0wGzAMBgNVHRMBAf8EAjAAMAsGA1UdDwQEAwIGwDAKBggqhkjOPQ"
    "QDAwNnADBkAjApHJGDhRBiz6c9ihMttcgY6C111Vz7CvxydeWIpEj306T/"
    "XVBVJB3EH7yGekP6r2sCMGodcksEfU8oOFSEFSBaecP7U0ybP1Z8swfkTAb4+"
    "sahtD6aIYAza7Mx2NgK9mizJ2hjYWJ1bmRsZYRZAhUwggIRMIIBlqADAgECAhEA+TF1aBuQr+"
    "EdRsy05Of4VjAKBggqhkjOPQQDAzBJMQswCQYDVQQGEwJVUzEPMA0GA1UECgwGQW1hem9uMQww"
    "CgYDVQQLDANBV1MxGzAZBgNVBAMMEmF3cy5uaXRyby1lbmNsYXZlczAeFw0xOTEwMjgxMzI4MD"
    "VaFw00OTEwMjgxNDI4MDVaMEkxCzAJBgNVBAYTAlVTMQ8wDQYDVQQKDAZBbWF6b24xDDAKBgNV"
    "BAsMA0FXUzEbMBkGA1UEAwwSYXdzLm5pdHJvLWVuY2xhdmVzMHYwEAYHKoZIzj0CAQYFK4EEAC"
    "IDYgAE/AJU66YIwfNocOKa2pC+RjgyknNuiUv/"
    "9nLZiURLUFHlNKSx9tvjwLxYGjK3sXYHDt4S1po/"
    "6iEbZudSz33R3QlfbxNw9BcIQ9ncEAEh5M9jASgJZkSHyXlihDBNxT/"
    "0o0IwQDAPBgNVHRMBAf8EBTADAQH/"
    "MB0GA1UdDgQWBBSQJbUN2QVH55bDlvpync+"
    "Zqd9LljAOBgNVHQ8BAf8EBAMCAYYwCgYIKoZIzj0EAwMDaQAwZgIxAKN/"
    "L5Ghyb1e57hifBaY0lUDjh8DQ/"
    "lbY6lijD05gJVFoR68vy47Vdiu7nG0w9at8wIxAKLzmxYFsnAopd1LoGm1AW5ltPvej+"
    "AGHWpTGX+c2vXZQ7xh/"
    "CvrA8tv7o0jAvPf9lkCwTCCAr0wggJEoAMCAQICECAdbYgjaUmdDWVObsq4SBEwCgYIKoZIzj0"
    "EAwMwSTELMAkGA1UEBhMCVVMxDzANBgNVBAoMBkFtYXpvbjEMMAoGA1UECwwDQVdTMRswGQYDV"
    "QQDDBJhd3Mubml0cm8tZW5jbGF2ZXMwHhcNMjIwNzI1MTg1MzAwWhcNMjIwODE0MTk1MzAwWjB"
    "kMQswCQYDVQQGEwJVUzEPMA0GA1UECgwGQW1hem9uMQwwCgYDVQQLDANBV1MxNjA0BgNVBAMML"
    "WFjYjU3MTkyMzAxMWE4NTAudXMtd2VzdC0yLmF3cy5uaXRyby1lbmNsYXZlczB2MBAGByqGSM4"
    "9AgEGBSuBBAAiA2IABJkgVZY7lF1WtvuWIasx6+09uTu5x1lT+"
    "vZocBLKSaXJvGGGO9p1ZMW6DrYuk5Yv6ErE/"
    "Lf3NRiS1JJBIcuoRK8kYi2CRVtZRKLk55oJdinDtjkVABnGBXLXXx1523ejZaOB1TCB0jASBgN"
    "VHRMBAf8ECDAGAQH/"
    "AgECMB8GA1UdIwQYMBaAFJAltQ3ZBUfnlsOW+"
    "nKdz5mp30uWMB0GA1UdDgQWBBSiIvnr04MZ5fElgXSJDyvp3UoV8jAOBgNVHQ8BAf8EBAMCAYY"
    "wbAYDVR0fBGUwYzBhoF+"
    "gXYZbaHR0cDovL2F3cy1uaXRyby1lbmNsYXZlcy1jcmwuczMuYW1hem9uYXdzLmNvbS9jcmwvY"
    "WI0OTYwY2MtN2Q2My00MmJkLTllOWYtNTkzMzhjYjY3Zjg0LmNybDAKBggqhkjOPQQDAwNnADB"
    "kAjA2fj8Q8ILYQnjSKLUW6vHLNwmAZ04Wyf/YG2CmbROA/"
    "+cxklLfhA09NCX76poJRVcCMCJZtiy6vhSmXMoiLJr8oN67XB1tbUdmljWK4KXrxcCcfPKImVD"
    "odSYRu5LRKEEsC1kDGTCCAxUwggKboAMCAQICEQDEVXvF+DtfOZvwDQc+"
    "fZc0MAoGCCqGSM49BAMDMGQxCzAJBgNVBAYTAlVTMQ8wDQYDVQQKDAZBbWF6b24xDDAKBgNVBA"
    "sMA0FXUzE2MDQGA1UEAwwtYWNiNTcxOTIzMDExYTg1MC51cy13ZXN0LTIuYXdzLm5pdHJvLWVu"
    "Y2xhdmVzMB4XDTIyMDcyNjEzMzEwNloXDTIyMDgwMTA1MzEwNlowgYkxPDA6BgNVBAMMM2I2OD"
    "U2MGFkZTQ1OTc5MWYuem9uYWwudXMtd2VzdC0yLmF3cy5uaXRyby1lbmNsYXZlczEMMAoGA1UE"
    "CwwDQVdTMQ8wDQYDVQQKDAZBbWF6b24xCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJXQTEQMA4GA1"
    "UEBwwHU2VhdHRsZTB2MBAGByqGSM49AgEGBSuBBAAiA2IABHcVbFGP4+"
    "IlBAnGnLyTLhbHihHvQSbmOglBkyqOXSb3gCESfHZyZgpQGnt3qgqjzOMF/uQe30hEODWHP/"
    "JeFsKuJZqNCR68PX5Se5T/"
    "fmlio+bPNKrqa+wAZXRHBdF4SqOB6jCB5zASBgNVHRMBAf8ECDAGAQH/"
    "AgEBMB8GA1UdIwQYMBaAFKIi+evTgxnl8SWBdIkPK+ndShXyMB0GA1UdDgQWBBRuH/"
    "UNZ8Jikmbu8xrm11NzhTFlKTAOBgNVHQ8BAf8EBAMCAYYwgYAGA1UdHwR5MHcwdaBzoHGGb2h0"
    "dHA6Ly9jcmwtdXMtd2VzdC0yLWF3cy1uaXRyby1lbmNsYXZlcy5zMy51cy13ZXN0LTIuYW1hem"
    "9uYXdzLmNvbS9jcmwvZDQ3NzYyN2EtMmI3YS00ZTEzLTk1N2MtN2U5Y2E2ZGYzMDc2LmNybDAK"
    "BggqhkjOPQQDAwNoADBlAjBKkbDPVKfdHvJ7MlpxD20JhZjno8qziS5iAOnoDaEN0h5QnogaZo"
    "rrWCL9Bbg+hLYCMQDvyInR1X5qQe13A5zmrnNhud+CabcHZd42jZ1vpyl0DvlLN8TUN+"
    "673NQlVTprwBlZAoMwggJ/"
    "MIICBaADAgECAhUA07T2LvAKjSTeOsUvVDHnc6gR12UwCgYIKoZIzj0EAwMwgYkxPDA6BgNVBA"
    "MMM2I2ODU2MGFkZTQ1OTc5MWYuem9uYWwudXMtd2VzdC0yLmF3cy5uaXRyby1lbmNsYXZlczEM"
    "MAoGA1UECwwDQVdTMQ8wDQYDVQQKDAZBbWF6b24xCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJXQT"
    "EQMA4GA1UEBwwHU2VhdHRsZTAeFw0yMjA3MjYxODM3MzJaFw0yMjA3MjcxODM3MzJaMIGOMQsw"
    "CQYDVQQGEwJVUzETMBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UEBwwHU2VhdHRsZTEPMA0GA1"
    "UECgwGQW1hem9uMQwwCgYDVQQLDANBV1MxOTA3BgNVBAMMMGktMDkzNjUxMmQ1YWU2ZWU5MWEu"
    "dXMtd2VzdC0yLmF3cy5uaXRyby1lbmNsYXZlczB2MBAGByqGSM49AgEGBSuBBAAiA2IABHMRuy"
    "/ZAl/8qrD9v2r7l0xrhw+OpFfHv5xnCQa4QY5X/NrbcJlElVNh1EBPXbLARsb/"
    "THbTtStKdEkBzLYqn3XKX+EbknOEf4vV5+"
    "0d3NpPn8lijNYabyRCi3LvK28DU6MmMCQwEgYDVR0TAQH/BAgwBgEB/"
    "wIBADAOBgNVHQ8BAf8EBAMCAgQwCgYIKoZIzj0EAwMDaAAwZQIwe9cKPHDV3S6Yq2rOpq9V0c0"
    "PGyjZY8s7mOOIZaMVFEXcHRKgEWm4w6Z1FcmfJ8NKAjEApQEpYHF4Leybt/"
    "Febgfuc50L8AA3QpiIk05gI/s/"
    "+YPHswScbJ3lcoFxK9HKlc6AanB1YmxpY19rZXlAaXVzZXJfZGF0YVggAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAABlbm9uY2VUgxSTNGtEEy4v+3mIvONR9ZPuuN1YYE546/"
    "617Xwu9L3LiwqknDCNG2AtS3lCoMHK9OXGBXIW0UixELEwQQZ2RpI5UCHrzP8lyEimgUrSJYLy"
    "FYtjFPH4dFwp5Fi37nQRFPd4irpcUP66OLKLpegJns+Pub6RzA==";

}  // namespace

class CoseSign1Test : public testing::Test {
 public:
  CoseSign1Test() = default;

 protected:
  std::vector<uint8_t> LoadExampleDocument() {
    return *base::Base64Decode(kExampleDocument);
  }

  cbor::Value ParseExampleDocument() {
    cbor::Reader::Config cbor_config;
    cbor_config.allow_and_canonicalize_out_of_order_keys = true;
    return *cbor::Reader::Read(LoadExampleDocument(), cbor_config);
  }

  std::vector<uint8_t> SerializeDocument(const cbor::Value& val) {
    return *cbor::Writer::Write(val);
  }

  CoseSign1 cose_sign1;
};

TEST_F(CoseSign1Test, DecodeFromExampleBytes) {
  EXPECT_TRUE(cose_sign1.payload().is_none());
  EXPECT_TRUE(cose_sign1.protected_headers().is_none());
  EXPECT_TRUE(cose_sign1.unprotected_headers().is_none());
  EXPECT_TRUE(cose_sign1.DecodeFromBytes(LoadExampleDocument()));

  EXPECT_TRUE(cose_sign1.protected_headers().is_map());

  const cbor::Value::MapValue& protected_headers_map =
      cose_sign1.protected_headers().GetMap();
  const cbor::Value::MapValue::const_iterator alg_value_it =
      protected_headers_map.find(cbor::Value(1));
  EXPECT_NE(alg_value_it, protected_headers_map.end());
  EXPECT_TRUE(alg_value_it->second.is_integer());

  EXPECT_TRUE(cose_sign1.unprotected_headers().is_map());

  EXPECT_TRUE(cose_sign1.payload().is_map());
  const cbor::Value::MapValue& payload_map = cose_sign1.payload().GetMap();
  // Verify a couple fields to ensure decoded payload is being stored.
  const cbor::Value::MapValue::const_iterator nonce_value_it =
      payload_map.find(cbor::Value("nonce"));
  const cbor::Value::MapValue::const_iterator cabundle_value_it =
      payload_map.find(cbor::Value("cabundle"));
  EXPECT_NE(nonce_value_it, payload_map.end());
  EXPECT_TRUE(nonce_value_it->second.is_bytestring());
  EXPECT_NE(cabundle_value_it, payload_map.end());
  EXPECT_TRUE(cabundle_value_it->second.is_array());
}

TEST_F(CoseSign1Test, BadLength) {
  cbor::Value parsed_document = ParseExampleDocument();
  std::vector<cbor::Value> doc_arr;
  std::transform(parsed_document.GetArray().begin(),
                 parsed_document.GetArray().end() - 1,
                 std::back_inserter(doc_arr),
                 [](const cbor::Value& val) { return val.Clone(); });

  EXPECT_FALSE(
      cose_sign1.DecodeFromBytes(SerializeDocument(cbor::Value(doc_arr))));

  EXPECT_TRUE(cose_sign1.payload().is_none());
  EXPECT_TRUE(cose_sign1.protected_headers().is_none());
  EXPECT_TRUE(cose_sign1.unprotected_headers().is_none());
}

TEST_F(CoseSign1Test, BadSignature) {
  cbor::Value parsed_document = ParseExampleDocument();
  std::vector<cbor::Value> doc_arr;
  std::transform(parsed_document.GetArray().begin(),
                 parsed_document.GetArray().end() - 1,
                 std::back_inserter(doc_arr),
                 [](const cbor::Value& val) { return val.Clone(); });
  doc_arr.emplace_back("bad signature");

  EXPECT_FALSE(
      cose_sign1.DecodeFromBytes(SerializeDocument(cbor::Value(doc_arr))));

  EXPECT_TRUE(cose_sign1.payload().is_map());
  EXPECT_TRUE(cose_sign1.protected_headers().is_map());
  EXPECT_TRUE(cose_sign1.unprotected_headers().is_map());
}

TEST_F(CoseSign1Test, BadRootType) {
  EXPECT_FALSE(
      cose_sign1.DecodeFromBytes(SerializeDocument(cbor::Value("bad doc"))));

  EXPECT_TRUE(cose_sign1.payload().is_none());
  EXPECT_TRUE(cose_sign1.protected_headers().is_none());
  EXPECT_TRUE(cose_sign1.unprotected_headers().is_none());
}

}  // namespace nitro_utils

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#define ParseJSON ParseJSON_ChromiumImpl
#define ParseCertificatesFile ParseCertificatesFile_ChromiumImpl
#include "src/net/tools/transport_security_state_generator/input_file_parsers.cc"
#undef ParseCertificatesFile
#undef ParseJSON

namespace {
// NOTE: Do not add any host which has TLS terminated by Cloudflare.
// Hosts backed by Universal SSL do not have stable CAs.
constexpr std::string_view kBravePinsJson = R"brave_pins_json({
  "pinsets": [
    {
      "name": "brave",
      "static_spki_hashes": [
        "AmazonRootCA1",
        "AmazonRootCA2",
        "AmazonRootCA3",
        "AmazonRootCA4",
        "StarfieldG2",
        "GlobalSignRootCA_R1",
        "GlobalSignRootCA_R3",
        "GlobalSignRootCA_R6",
        "GlobalSignRootCA_R46",
        "GlobalSignRootCA_R5",
        "GlobalSignRootCA_E46",
        "ISRGRootCA_X1",
        "ISRGRootCA_X2"
      ]
    }
  ],
  "entries": [
    // Brave
    { "name": "adblock-data.s3.brave.com", "pins": "brave"},
    { "name": "ai-chat.bsg.brave.com", "pins": "brave"},
    { "name": "brave-core-ext.s3.brave.com", "pins": "brave"},
    { "name": "brave-today-cdn.brave.com", "pins": "brave"},
    { "name": "clients4.brave.com", "pins": "brave"},
    { "name": "componentupdater.brave.com", "pins": "brave"},
    { "name": "crxdownload.brave.com", "pins": "brave"},
    { "name": "devtools.brave.com", "pins": "brave"},
    { "name": "dict.brave.com", "pins": "brave"},
    { "name": "extensionupdater.brave.com", "pins": "brave"},
    { "name": "feedback.brave.com", "pins": "brave"},
    { "name": "gaia.brave.com", "pins": "brave"},
    { "name": "go-updater.brave.com", "pins": "brave"},
    { "name": "mobile-data.s3.brave.com", "pins": "brave"},
    { "name": "pcdn.brave.com", "pins": "brave"},
    { "name": "redirector.brave.com", "pins": "brave"},
    { "name": "safebrowsing.brave.com", "pins": "brave"},
    { "name": "safebrowsing2.brave.com", "pins": "brave"},
    { "name": "sb-ssl.brave.com", "pins": "brave"},
    { "name": "static.brave.com", "pins": "brave"},
    { "name": "static1.brave.com", "pins": "brave"},
    { "name": "sync-v2.brave.com", "pins": "brave"},
    { "name": "sync-v2.brave.software", "pins": "brave"},
    { "name": "sync-v2.bravesoftware.com", "pins": "brave"},
    { "name": "tor.bravesoftware.com", "pins": "brave"},
    { "name": "translate.brave.com", "pins": "brave"},
    { "name": "translate-static.brave.com", "pins": "brave"},
    { "name": "variations.brave.com", "pins": "brave"},

    // P2A/P3A
    { "name": "collector.bsg.brave.com", "pins": "brave"},
    { "name": "p2a.brave.com", "pins": "brave"},
    { "name": "p2a-json.brave.com", "pins": "brave"},
    { "name": "p3a.brave.com", "pins": "brave"},
    { "name": "p3a-creative.brave.com", "pins": "brave"},
    { "name": "p3a-json.brave.com", "pins": "brave"},
    { "name": "p3a.bravesoftware.com", "pins": "brave"},
    { "name": "p3a-dev.bravesoftware.com", "pins": "brave"},
    { "name": "star-randsrv.bsg.brave.com", "pins": "brave"},

    // Creators
    { "name": "creators.basicattentiontoken.org", "pins": "brave"},
    { "name": "creators.brave.com", "pins": "brave"},
    { "name": "publishers.basicattentiontoken.org", "pins": "brave"},
    { "name": "publishers.brave.com", "pins": "brave"},

    // Wallet
    { "name": "goerli-infura.brave.com", "pins": "brave"},
    { "name": "sepolia-infura.brave.com", "pins": "brave"},
    { "name": "mainnet-infura.brave.com", "pins": "brave"},
    { "name": "mainnet-beta-solana.brave.com", "pins": "brave"},
    { "name": "mainnet-polygon.brave.com", "pins": "brave"},

    // Rewards
    { "name": "anonymous.ads.brave.com", "pins": "brave"},
    { "name": "anonymous.ads.bravesoftware.com", "pins": "brave"},
    { "name": "api.rewards.brave.com", "pins": "brave"},
    { "name": "api.rewards.bravesoftware.com", "pins": "brave"},
    { "name": "api.rewards.brave.software", "pins": "brave"},
    { "name": "geo.ads.brave.com", "pins": "brave"},
    { "name": "geo.ads.bravesoftware.com", "pins": "brave"},
    { "name": "grant.rewards.brave.com", "pins": "brave"},
    { "name": "grant.rewards.bravesoftware.com", "pins": "brave"},
    { "name": "grant.rewards.brave.software", "pins": "brave"},
    { "name": "mywallet.ads.brave.com", "pins": "brave"},
    { "name": "mywallet.ads.bravesoftware.com", "pins": "brave"},
    { "name": "payment.rewards.brave.com", "pins": "brave"},
    { "name": "payment.rewards.bravesoftware.com", "pins": "brave"},
    { "name": "payment.rewards.brave.software", "pins": "brave"},
    { "name": "rewards.brave.com", "pins": "brave"},
    { "name": "search.anonymous.brave.com", "pins": "brave"},
    { "name": "search.anonymous.bravesoftware.com", "pins": "brave"},
    { "name": "static.ads.brave.com", "pins": "brave"},
    { "name": "static.ads.bravesoftware.com", "pins": "brave"},

    // Search
    { "name": "search.brave.com", "pins": "brave"},
    { "name": "cdn.search.brave.com", "pins": "brave"},
    { "name": "fg.search.brave.com", "pins": "brave"},
    { "name": "imgs.search.brave.com", "pins": "brave"},
    { "name": "tiles.search.brave.com", "pins": "brave"},
    { "name": "collector.wdp.brave.com", "pins": "brave"},
    { "name": "patterns.wdp.brave.com", "pins": "brave"},
    { "name": "quorum.wdp.brave.com", "pins": "brave"},
    { "name": "star.wdp.brave.com", "pins": "brave"},

    // Premium
    { "name": "account.brave.com", "pins": "brave"},
    { "name": "account.bravesoftware.com", "pins": "brave"},
    { "name": "account.brave.software", "pins": "brave"},

    // Test page using a CA outside of the pinset (expected to be blocked)
    { "name": "ssl-pinning.someblog.org", "pins" : "brave"}
 ]})brave_pins_json";

constexpr std::string_view kBraveHstsJson = R"brave_hsts_json({
  "entries": [
    // Critical endpoints that should remain unpinned so that they
    // always work.
    {
      "name": "updates.bravesoftware.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "updates-cdn.bravesoftware.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "usage-ping.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },

    // Brave
    {
      "name": "adblock-data.s3.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "ai-chat.bsg.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "brave-core-ext.s3.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "brave-today-cdn.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "clients4.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "componentupdater.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "crxdownload.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "devtools.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "dict.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "extensionupdater.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "feedback.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "gaia.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "go-updater.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "mobile-data.s3.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "p2a.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "p2a-json.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "p3a.brave.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "p3a-creative.brave.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "p3a-json.brave.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "p3a.bravesoftware.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "p3a-dev.bravesoftware.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "pcdn.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "redirector.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "safebrowsing.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "safebrowsing2.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "sb-ssl.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "static.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "static1.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "sync-v2.brave.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "sync-v2.brave.software",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "sync-v2.bravesoftware.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "tor.bravesoftware.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "translate.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "translate-static.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "variations.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },

    // Creators
    {
      "name": "creators.basicattentiontoken.org",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "creators.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "publishers.basicattentiontoken.org",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "publishers.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },

    // Wallet
    {
      "name": "goerli-infura.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "sepolia-infura.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "mainnet-infura.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "mainnet-beta-solana.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "mainnet-polygon.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },

    // Rewards
    {
      "name": "anonymous.ads.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "anonymous.ads.bravesoftware.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "api.rewards.brave.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "api.rewards.bravesoftware.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "api.rewards.brave.software",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "geo.ads.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "geo.ads.bravesoftware.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "grant.rewards.brave.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "grant.rewards.bravesoftware.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "grant.rewards.brave.software",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "mywallet.ads.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "mywallet.ads.bravesoftware.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "payment.rewards.brave.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "payment.rewards.bravesoftware.com",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "payment.rewards.brave.software",
      "policy": "custom",
      "mode": "force-https"
    },
    {
      "name": "rewards.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "search.anonymous.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "search.anonymous.bravesoftware.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "static.ads.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "static.ads.bravesoftware.com",
      "mode": "force-https",
      "policy": "custom"
    },

    // Search
    {
      "name": "search.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "cdn.search.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "fg.search.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "imgs.search.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "tiles.search.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "collector.wdp.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "patterns.wdp.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "quorum.wdp.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "star.wdp.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },

    // Premium
    {
      "name": "account.brave.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "account.bravesoftware.com",
      "mode": "force-https",
      "policy": "custom"
    },
    {
      "name": "account.brave.software",
      "mode": "force-https",
      "policy": "custom"
    }
 ]})brave_hsts_json";
}  // namespace

namespace net::transport_security_state {

bool ParseCertificatesFile(std::string_view certs_input,
                           Pinsets* pinsets,
                           base::Time* timestamp) {
  constexpr std::string_view brave_certs = R"brave_certs(
# Last updated: Thu Dec 19 19:49:53 UTC 2024
PinsListTimestamp
1734637793

# =====BEGIN BRAVE ROOTS ASC=====
#From https://www.amazontrust.com/repository/
AmazonRootCA1
-----BEGIN CERTIFICATE-----
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
-----END CERTIFICATE-----

AmazonRootCA2
-----BEGIN CERTIFICATE-----
MIIFQTCCAymgAwIBAgITBmyf0pY1hp8KD+WGePhbJruKNzANBgkqhkiG9w0BAQwF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAyMB4XDTE1MDUyNjAwMDAwMFoXDTQwMDUyNjAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMjCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK2Wny2cSkxK
gXlRmeyKy2tgURO8TW0G/LAIjd0ZEGrHJgw12MBvIITplLGbhQPDW9tK6Mj4kHbZ
W0/jTOgGNk3Mmqw9DJArktQGGWCsN0R5hYGCrVo34A3MnaZMUnbqQ523BNFQ9lXg
1dKmSYXpN+nKfq5clU1Imj+uIFptiJXZNLhSGkOQsL9sBbm2eLfq0OQ6PBJTYv9K
8nu+NQWpEjTj82R0Yiw9AElaKP4yRLuH3WUnAnE72kr3H9rN9yFVkE8P7K6C4Z9r
2UXTu/Bfh+08LDmG2j/e7HJV63mjrdvdfLC6HM783k81ds8P+HgfajZRRidhW+me
z/CiVX18JYpvL7TFz4QuK/0NURBs+18bvBt+xa47mAExkv8LV/SasrlX6avvDXbR
8O70zoan4G7ptGmh32n2M8ZpLpcTnqWHsFcQgTfJU7O7f/aS0ZzQGPSSbtqDT6Zj
mUyl+17vIWR6IF9sZIUVyzfpYgwLKhbcAS4y2j5L9Z469hdAlO+ekQiG+r5jqFoz
7Mt0Q5X5bGlSNscpb/xVA1wf+5+9R+vnSUeVC06JIglJ4PVhHvG/LopyboBZ/1c6
+XUyo05f7O0oYtlNc/LMgRdg7c3r3NunysV+Ar3yVAhU/bQtCSwXVEqY0VThUWcI
0u1ufm8/0i2BWSlmy5A5lREedCf+3euvAgMBAAGjQjBAMA8GA1UdEwEB/wQFMAMB
Af8wDgYDVR0PAQH/BAQDAgGGMB0GA1UdDgQWBBSwDPBMMPQFWAJI/TPlUq9LhONm
UjANBgkqhkiG9w0BAQwFAAOCAgEAqqiAjw54o+Ci1M3m9Zh6O+oAA7CXDpO8Wqj2
LIxyh6mx/H9z/WNxeKWHWc8w4Q0QshNabYL1auaAn6AFC2jkR2vHat+2/XcycuUY
+gn0oJMsXdKMdYV2ZZAMA3m3MSNjrXiDCYZohMr/+c8mmpJ5581LxedhpxfL86kS
k5Nrp+gvU5LEYFiwzAJRGFuFjWJZY7attN6a+yb3ACfAXVU3dJnJUH/jWS5E4ywl
7uxMMne0nxrpS10gxdr9HIcWxkPo1LsmmkVwXqkLN1PiRnsn/eBG8om3zEK2yygm
btmlyTrIQRNg91CMFa6ybRoVGld45pIq2WWQgj9sAq+uEjonljYE1x2igGOpm/Hl
urR8FLBOybEfdF849lHqm/osohHUqS0nGkWxr7JOcQ3AWEbWaQbLU8uz/mtBzUF+
fUwPfHJ5elnNXkoOrJupmHN5fLT0zLm4BwyydFy4x2+IoZCn9Kr5v2c69BoVYh63
n749sSmvZ6ES8lgQGVMDMBu4Gon2nL2XA46jCfMdiyHxtN/kHNGfZQIG6lzWE7OE
76KlXIx3KadowGuuQNKotOrN8I1LOJwZmhsoVLiJkO/KdYE+HvJkJMcYr07/R54H
9jVlpNMKVv/1F2Rs76giJUmTtt8AF9pYfl3uxRuw0dFfIRDH+fO6AgonB8Xx1sfT
4PsJYGw=
-----END CERTIFICATE-----

AmazonRootCA3
-----BEGIN CERTIFICATE-----
MIIBtjCCAVugAwIBAgITBmyf1XSXNmY/Owua2eiedgPySjAKBggqhkjOPQQDAjA5
MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6b24g
Um9vdCBDQSAzMB4XDTE1MDUyNjAwMDAwMFoXDTQwMDUyNjAwMDAwMFowOTELMAkG
A1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJvb3Qg
Q0EgMzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABCmXp8ZBf8ANm+gBG1bG8lKl
ui2yEujSLtf6ycXYqm0fc4E7O5hrOXwzpcVOho6AF2hiRVd9RFgdszflZwjrZt6j
QjBAMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgGGMB0GA1UdDgQWBBSr
ttvXBp43rDCGB5Fwx5zEGbF4wDAKBggqhkjOPQQDAgNJADBGAiEA4IWSoxe3jfkr
BqWTrBqYaGFy+uGh0PsceGCmQ5nFuMQCIQCcAu/xlJyzlvnrxir4tiz+OpAUFteM
YyRIHN8wfdVoOw==
-----END CERTIFICATE-----

AmazonRootCA4
-----BEGIN CERTIFICATE-----
MIIB8jCCAXigAwIBAgITBmyf18G7EEwpQ+Vxe3ssyBrBDjAKBggqhkjOPQQDAzA5
MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6b24g
Um9vdCBDQSA0MB4XDTE1MDUyNjAwMDAwMFoXDTQwMDUyNjAwMDAwMFowOTELMAkG
A1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJvb3Qg
Q0EgNDB2MBAGByqGSM49AgEGBSuBBAAiA2IABNKrijdPo1MN/sGKe0uoe0ZLY7Bi
9i0b2whxIdIA6GO9mif78DluXeo9pcmBqqNbIJhFXRbb/egQbeOc4OO9X4Ri83Bk
M6DLJC9wuoihKqB1+IGuYgbEgds5bimwHvouXKNCMEAwDwYDVR0TAQH/BAUwAwEB
/zAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0OBBYEFNPsxzplbszh2naaVvuc84ZtV+WB
MAoGCCqGSM49BAMDA2gAMGUCMDqLIfG9fhGt0O9Yli/W651+kI0rz2ZVwyzjKKlw
CkcO8DdZEv8tmZQoTipPNU0zWgIxAOp1AE47xDqUEpHJWEadIRNyp4iciuRMStuW
1KyLa2tJElMzrdfkviT8tQp21KW8EA==
-----END CERTIFICATE-----

StarfieldG2
-----BEGIN CERTIFICATE-----
MIID7zCCAtegAwIBAgIBADANBgkqhkiG9w0BAQsFADCBmDELMAkGA1UEBhMCVVMx
EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxJTAjBgNVBAoT
HFN0YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xOzA5BgNVBAMTMlN0YXJmaWVs
ZCBTZXJ2aWNlcyBSb290IENlcnRpZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5
MDkwMTAwMDAwMFoXDTM3MTIzMTIzNTk1OVowgZgxCzAJBgNVBAYTAlVTMRAwDgYD
VQQIEwdBcml6b25hMRMwEQYDVQQHEwpTY290dHNkYWxlMSUwIwYDVQQKExxTdGFy
ZmllbGQgVGVjaG5vbG9naWVzLCBJbmMuMTswOQYDVQQDEzJTdGFyZmllbGQgU2Vy
dmljZXMgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIwDQYJKoZI
hvcNAQEBBQADggEPADCCAQoCggEBANUMOsQq+U7i9b4Zl1+OiFOxHz/Lz58gE20p
OsgPfTz3a3Y4Y9k2YKibXlwAgLIvWX/2h/klQ4bnaRtSmpDhcePYLQ1Ob/bISdm2
8xpWriu2dBTrz/sm4xq6HZYuajtYlIlHVv8loJNwU4PahHQUw2eeBGg6345AWh1K
Ts9DkTvnVtYAcMtS7nt9rjrnvDH5RfbCYM8TWQIrgMw0R9+53pBlbQLPLJGmpufe
hRhJfGZOozptqbXuNC66DQO4M99H67FrjSXZm86B0UVGMpZwh94CDklDhbZsc7tk
6mFBrMnUVN+HL8cisibMn1lUaJ/8viovxFUcdUBgF4UCVTmLfwUCAwEAAaNCMEAw
DwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYEFJxfAN+q
AdcwKziIorhtSpzyEZGDMA0GCSqGSIb3DQEBCwUAA4IBAQBLNqaEd2ndOxmfZyMI
bw5hyf2E3F/YNoHN2BtBLZ9g3ccaaNnRbobhiCPPE95Dz+I0swSdHynVv/heyNXB
ve6SbzJ08pGCL72CQnqtKrcgfU28elUSwhXqvfdqlS5sdJ/PHLTyxQGjhdByPq1z
qwubdQxtRbeOlKyWN7Wg0I8VRw7j6IPdj/3vQQF3zCepYoUz8jcI73HPdwbeyBkd
iEDPfUYd/x7H4c7/I9vG+o1VTqkC50cRRj70/b17KSa7qWFiNyi2LSr2EIZkyXCn
0q23KXB56jzaYyWf/Wi3MOxw+3WKt21gZ7IeyLnp2KhvAotnDU0mV3HaIPzBSlCN
sSi6
-----END CERTIFICATE-----

# From https://support.globalsign.com/ca-certificates/root-certificates/globalsign-root-certificates
GlobalSignRootCA_R1
-----BEGIN CERTIFICATE-----
MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG
A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv
b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw
MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i
YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT
aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ
jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp
xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp
1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG
snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ
U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8
9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E
BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B
AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz
yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE
38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP
AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad
DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME
HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==
-----END CERTIFICATE-----

GlobalSignRootCA_R3
-----BEGIN CERTIFICATE-----
MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4
GA1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbF
NpZ24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwM
zE4MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzET
MBEGA1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQY
JKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2Ec
WtiHL8RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUh
hB5uzsTgHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL
0gRgykmmKPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65
TpjoWc4zdQQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rU
AVSNECMWEZXriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCA
wEAAaNCMEAwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0O
BBYEFI/wS3+oLkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNv
AUKr+yAzv95ZURUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8
dEe3jgr25sbwMpjjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw
8lo/s7awlOqzJCK6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0
095MJ6RMG3NzdvQXmcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVE
TI53O9zJrlAGomecsMx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02
JQZR7rkpeDMdmztcpHWD9f
-----END CERTIFICATE-----

GlobalSignRootCA_R6
-----BEGIN CERTIFICATE-----
MIIFgzCCA2ugAwIBAgIORea7A4Mzw4VlSOb/RVEwDQYJKoZIhvcNAQEMBQAwTDE
gMB4GA1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjYxEzARBgNVBAoTCkdsb2
JhbFNpZ24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMTQxMjEwMDAwMDAwWhcNM
zQxMjEwMDAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBS
NjETMBEGA1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCAiI
wDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAJUH6HPKZvnsFMp7PPcNCPG0RQ
ssgrRIxutbPK6DuEGSMxSkb3/pKszGsIhrxbaJ0cay/xTOURQh7ErdG1rG1ofuT
ToVBu1kZguSgMpE3nOUTvOniX9PeGMIyBJQbUJmL025eShNUhqKGoC3GYEOfsSK
vGRMIRxDaNc9PIrFsmbVkJq3MQbFvuJtMgamHvm566qjuL++gmNQ0PAYid/kD3n
16qIfKtJwLnvnvJO7bVPiSHyMEAc4/2ayd2F+4OqMPKq0pPbzlUoSB239jLKJz9
CgYXfIWHSw1CM69106yqLbnQneXUQtkPGBzVeS+n68UARjNN9rkxi+azayOeSsJ
Da38O+2HBNXk7besvjihbdzorg1qkXy4J02oW9UivFyVm4uiMVRQkQVlO6jxTiW
m05OWgtH8wY2SXcwvHE35absIQh1/OZhFj931dmRl4QKbNQCTXTAFO39OfuD8l4
UoQSwC+n+7o/hbguyCLNhZglqsQY6ZZZZwPA1/cnaKI0aEYdwgQqomnUdnjqGBQ
Ce24DWJfncBZ4nWUx2OVvq+aWh2IMP0f/fMBH5hc8zSPXKbWQULHpYT9NLCEnFl
WQaYw55PfWzjMpYrZxCRXluDocZXFSxZba/jJvcE+kNb7gu3GduyYsRtYQUigAZ
cIN5kZeR1BonvzceMgfYFGM8KEyvAgMBAAGjYzBhMA4GA1UdDwEB/wQEAwIBBjA
PBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBSubAWjkxPioufi1xzWx/B/yGdToD
AfBgNVHSMEGDAWgBSubAWjkxPioufi1xzWx/B/yGdToDANBgkqhkiG9w0BAQwFA
AOCAgEAgyXt6NH9lVLNnsAEoJFp5lzQhN7craJP6Ed41mWYqVuoPId8AorRbrcW
c+ZfwFSY1XS+wc3iEZGtIxg93eFyRJa0lV7Ae46ZeBZDE1ZXs6KzO7V33EByrKP
rmzU+sQghoefEQzd5Mr6155wsTLxDKZmOMNOsIeDjHfrYBzN2VAAiKrlNIC5waN
rlU/yDXNOd8v9EDERm8tLjvUYAGm0CuiVdjaExUd1URhxN25mW7xocBFymFe944
Hn+Xds+qkxV/ZoVqW/hpvvfcDDpw+5CRu3CkwWJ+n1jez/QcYF8AOiYrg54NMMl
+68KnyBr3TsTjxKM4kEaSHpzoHdpx7Zcf4LIHv5YGygrqGytXm3ABdJ7t+uA/iU
3/gKbaKxCXcPu9czc8FB10jZpnOZ7BN9uBmm23goJSFmH63sUYHpkqmlD75HHTO
wY3WzvUy2MmeFe8nI+z1TIvWfspA9MRf/TuTAjB0yPEL+GltmZWrSZVxykzLsVi
VO6LAUP5MSeGbEYNNVMnbrt9x+vJJUEeKgDu+6B5dpffItKoZB0JaezPkvILFa9
x8jvOOJckvB595yEunQtYQEgfn7R8k8HWV+LLUNS60YMlOH1Zkd5d9VUWx+tJDf
LRVpOoERIyNiwmcUVhAn21klJwGW45hpxbqCo8YLoRT5s1gLXCmeDBVrJpBA=
-----END CERTIFICATE-----

GlobalSignRootCA_R46
-----BEGIN CERTIFICATE-----
MIIFWjCCA0KgAwIBAgISEdK7udcjGJ5AXwqdLdDfJWfRMA0GCSqGSIb3DQEBDAU
AMEYxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9iYWxTaWduIG52LXNhMRwwGg
YDVQQDExNHbG9iYWxTaWduIFJvb3QgUjQ2MB4XDTE5MDMyMDAwMDAwMFoXDTQ2M
DMyMDAwMDAwMFowRjELMAkGA1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24g
bnYtc2ExHDAaBgNVBAMTE0dsb2JhbFNpZ24gUm9vdCBSNDYwggIiMA0GCSqGSIb
3DQEBAQUAA4ICDwAwggIKAoICAQCsrHQy6LNl5brtQyYdpokNRbopiLKkHWPd08
EsCVeJOaFV6Wc0dwxu5FUdUiXSE2te4R2pt32JMl8Nnp8semNgQB+msLZ4j5lUl
ghYruQGvGIFAha/r6gjA7aUD7xubMLL1aa7DOn2wQL7Id5m3RerdELv8HQvJfTq
a1VbkNud316HCkD7rRlr+/fKYIje2sGP1q7Vf9Q8g+7XFkyDRTNrJ9CG0Bwta/O
rffGFqfUo0q3v84RLHIf8E6M6cqJaESvWJ3En7YEtbWaBkoe0G1h6zD8K+kZPT
Xhc+CtI4wSEy132tGqzZfxCnlEmIyDLPRT5ge1lFgBPGmSXZgjPjHvjK8Cd+RTy
G/FWaha/LIWFzXg4mutCagI0GIMXTpRW+LaCtfOW3T3zvn8gdz57GSNrLNRyc0N
XfeD412lPFzYE+cCQYDdF3uYM2HSNrpyibXRdQr4G9dlkbgIQrImwTDsHTUB+JM
WKmIJ5jqSngiCNI/onccnfxkF0oE32kRbcRoxfKWMxWXEM2G/CtjJ9++ZdU6Z+F
fy7dXxd7Pj2Fxzsx2sZy/N78CsHpdlseVR2bJ0cpm4O6XkMqCNqo98bMDGfsVR7
/mrLZqrcZdCinkqaByFrgY/bxFn63iLABJzjqls2k+g9vXqhnQt2sQvHnf3PmKg
Gwvgqo6GDoLclcqUC4wIDAQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQ
H/BAUwAwEB/zAdBgNVHQ4EFgQUA1yrc4GHqMywptWU4jaWSf8FmSwwDQYJKoZIh
vcNAQEMBQADggIBAHx47PYCLLtbfpIrXTncvtgdokIzTfnvpCo7RGkerNlFo048
p9gkUbJUHJNOxO97k4VgJuoJSOD1u8fpaNK7ajFxzHmuEajwmf3lH7wvqMxX63b
EIaZHU1VNaL8FpO7XJqti2kM3S+LGteWygxk6x9PbTZ4IevPuzz5i+6zoYMzRx6
Fcg0XERczzF2sUyQQCPtIkpnnpHs6i58FZFZ8d4kuaPp92CC1r2LpXFNqD6v6MV
enQTqnMdzGxRBF6XLE+0xRFFRhiJBPSy03OXIPBNvIQtQ6IbbjhVp+J3pZmOUdk
LG5NrmJ7v2B0GbhWrJKsFjLtrWhV/pi60zTe9Mlhww6G9kuEYO4Ne7UyWHmRVSy
BQ7N0H3qqJZ4d16GLuc1CLgSkZoNNiTW2bKg2SnkheCLQQrzRQDGQob4Ez8pn7f
XwgNNgyYMqIgXQBztSvwyeqiv5u+YfjyW6hY0XHgL+XVAEV8/+LbzvXMAaq7afJ
Mbfc2hIkCwU9D9SGuTSyxTDYWnP4vkYxboznxSjBF25cfe1lNj2M8FawTSLfJvd
kzrnE6JwYZ+vj+vYxXX4M2bUdGc6N3ec592kD3ZDZopD8p/7DEJ4Y9HiD2971KE
9dJeFt0g5QdYg/NA6s/rob8SKunE3vouXsXgxT7PntgMTzlSdriVZzH81Xwj3QE
UxeCp6
-----END CERTIFICATE-----

GlobalSignRootCA_R5
-----BEGIN CERTIFICATE-----
MIICHjCCAaSgAwIBAgIRYFlJ4CYuu1X5CneKcflK2GwwCgYIKoZIzj0EAwMwUDE
kMCIGA1UECxMbR2xvYmFsU2lnbiBFQ0MgUm9vdCBDQSAtIFI1MRMwEQYDVQQKEw
pHbG9iYWxTaWduMRMwEQYDVQQDEwpHbG9iYWxTaWduMB4XDTEyMTExMzAwMDAwM
FoXDTM4MDExOTAzMTQwN1owUDEkMCIGA1UECxMbR2xvYmFsU2lnbiBFQ0MgUm9v
dCBDQSAtIFI1MRMwEQYDVQQKEwpHbG9iYWxTaWduMRMwEQYDVQQDEwpHbG9iYWx
TaWduMHYwEAYHKoZIzj0CAQYFK4EEACIDYgAER0UOlvt9Xb/pOdEh+J8LttV7Hp
I6SFkc8GIxLcB6KP4ap1yztsyX50XUWPrRd21DosCHZTQKH3rd6zwzocWdTaRvQ
ZU4f8kehOvRnkmSh5SHDDqFSmafnVmTTZdhBoZKo0IwQDAOBgNVHQ8BAf8EBAMC
AQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUPeYpSJvqB8ohREom3m7e0oP
Qn1kwCgYIKoZIzj0EAwMDaAAwZQIxAOVpEslu28YxuglB4Zf4+/2a4n0Sye18ZN
PLBSWLVtmg515dTguDnFt2KaAJJiFqYgIwcdK1j1zqO+F4CYWodZI7yFz9SO8Nd
CKoCOJuxUnOxwy8p2Fp8fc74SrL+SvzZpA3
-----END CERTIFICATE-----

GlobalSignRootCA_E46
-----BEGIN CERTIFICATE-----
MIICCzCCAZGgAwIBAgISEdK7ujNu1LzmJGjFDYQdmOhDMAoGCCqGSM49BAMDME
YxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9iYWxTaWduIG52LXNhMRwwGgYD
VQQDExNHbG9iYWxTaWduIFJvb3QgRTQ2MB4XDTE5MDMyMDAwMDAwMFoXDTQ2MD
MyMDAwMDAwMFowRjELMAkGA1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24g
bnYtc2ExHDAaBgNVBAMTE0dsb2JhbFNpZ24gUm9vdCBFNDYwdjAQBgcqhkjOPQ
IBBgUrgQQAIgNiAAScDrHPt+ieUnd1NPqlRqetMhkytAepJ8qUuwzSChDH2omw
lwxwEwkBjtjqR+q+soArzfwoDdusvKSGN+1wCAB16pMLey5SnCNoIwZD7JIvU4
Tb+0cUB+hflGddyXqBPCCjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8E
BTADAQH/MB0GA1UdDgQWBBQxCpCPtsad0kRLgLWi5h+xEk8blTAKBggqhkjOPQ
QDAwNoADBlAjEA31SQ7Zvvi5QCkxeCmb6zniz2C5GMn0oUsfZkvLtoURMMA/cV
i4RguYv/Uo7njLwcAjA8+RHUjE7AwWHCFUyqqx0LMV87HOIAl0Qx5v5zli/alt
P+CAezNIm8BZ/3Hobui3A=
-----END CERTIFICATE-----

# From https://letsencrypt.org/certificates/#root-certificates
ISRGRootCA_X1
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----

ISRGRootCA_X2
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
# =====END BRAVE ROOTS ASC=====
)brave_certs";

  return ParseCertificatesFile_ChromiumImpl(brave_certs, pinsets, timestamp);
}

bool ParseJSON(std::string_view hsts_json,
               std::string_view pins_json,
               TransportSecurityStateEntries* entries,
               Pinsets* pinsets) {
  Pinsets chromium_pinsets;
  TransportSecurityStateEntries chromium_entries;
  if (!ParseJSON_ChromiumImpl(hsts_json, pins_json, &chromium_entries,
                              &chromium_pinsets)) {
    return false;
  }

  for (auto& entry : chromium_entries) {
    // Google has asked us not to include the pins that ship with Chrome,
    // but we do want the preloaded HSTS entries.
    entry->pinset = "";
    if (!entry->force_https) {
      continue;
    }

    entries->push_back(std::move(entry));
  }

  return ParseJSON_ChromiumImpl(kBraveHstsJson, kBravePinsJson, entries,
                                pinsets);
}

}  // namespace net::transport_security_state

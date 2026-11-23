// Copyright (c) 2016, Google Inc.
// SPDX-License-Identifier: ISC

#include <gtest/gtest.h>

#include <openssl/asn1.h>
#include <openssl/bytestring.h>
#include <openssl/crypto.h>
#include <openssl/obj.h>

#include "../internal.h"


TEST(ObjTest, TestBasic) {
  static const int kNID = NID_sha256WithRSAEncryption;
  static const char kShortName[] = "RSA-SHA256";
  static const char kLongName[] = "sha256WithRSAEncryption";
  static const char kText[] = "1.2.840.113549.1.1.11";
  static const uint8_t kDER[] = {
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b,
  };

  CBS cbs;
  CBS_init(&cbs, kDER, sizeof(kDER));
  ASSERT_EQ(kNID, OBJ_cbs2nid(&cbs));
  ASSERT_EQ(kNID, OBJ_sn2nid(kShortName));
  ASSERT_EQ(kNID, OBJ_ln2nid(kLongName));
  ASSERT_EQ(kNID, OBJ_txt2nid(kShortName));
  ASSERT_EQ(kNID, OBJ_txt2nid(kLongName));
  ASSERT_EQ(kNID, OBJ_txt2nid(kText));

  ASSERT_STREQ(kShortName, OBJ_nid2sn(kNID));
  ASSERT_STREQ(kLongName, OBJ_nid2ln(kNID));

  ASSERT_EQ(NID_undef, OBJ_sn2nid("this is not an OID"));
  ASSERT_EQ(NID_undef, OBJ_ln2nid("this is not an OID"));
  ASSERT_EQ(NID_undef, OBJ_txt2nid("this is not an OID"));

  CBS_init(&cbs, NULL, 0);
  ASSERT_EQ(NID_undef, OBJ_cbs2nid(&cbs));

  // 1.2.840.113554.4.1.72585.2 (https://davidben.net/oid).
  static const uint8_t kUnknownDER[] = {
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x12, 0x04, 0x01, 0x84, 0xb7, 0x09, 0x02,
  };
  CBS_init(&cbs, kUnknownDER, sizeof(kUnknownDER));
  ASSERT_EQ(NID_undef, OBJ_cbs2nid(&cbs));

  EXPECT_EQ(NID_undef, OBJ_sn2nid("UNDEF"));
  EXPECT_EQ(NID_undef, OBJ_ln2nid("undefined"));
  EXPECT_EQ(OBJ_get_undef(), OBJ_nid2obj(NID_undef));
}

// NID_ct_precert_scts (RFC 6962, section 3.3) is registered only so that
// certificate printers show its name instead of a raw OID.
TEST(ObjTest, CTPrecertSCTs) {
  static const int kNID = NID_ct_precert_scts;
  static const char kShortName[] = "ct_precert_scts";
  static const char kLongName[] = "CT Precertificate SCTs";
  static const char kText[] = "1.3.6.1.4.1.11129.2.4.2";
  static const uint8_t kDER[] = {
      0x2b, 0x06, 0x01, 0x04, 0x01, 0xd6, 0x79, 0x02, 0x04, 0x02,
  };

  CBS cbs;
  CBS_init(&cbs, kDER, sizeof(kDER));
  EXPECT_EQ(kNID, OBJ_cbs2nid(&cbs));
  EXPECT_EQ(kNID, OBJ_sn2nid(kShortName));
  EXPECT_EQ(kNID, OBJ_ln2nid(kLongName));
  EXPECT_EQ(kNID, OBJ_txt2nid(kShortName));
  EXPECT_EQ(kNID, OBJ_txt2nid(kLongName));
  EXPECT_EQ(kNID, OBJ_txt2nid(kText));

  EXPECT_STREQ(kShortName, OBJ_nid2sn(kNID));
  EXPECT_STREQ(kLongName, OBJ_nid2ln(kNID));

  // OBJ_obj2txt prefers the long name over the dotted OID, which is what
  // X509V3_extensions_print uses for the extension label.
  char buf[32];
  ASSERT_GT(OBJ_obj2txt(buf, sizeof(buf), OBJ_nid2obj(kNID), 0), 0);
  EXPECT_STREQ(kLongName, buf);
}

// ML-DSA uses the compact "MLDSAxx" short name but the OpenSSL 3.5 long name
// "ML-DSA-xx" (see crypto/obj/objects.txt). Both spellings must resolve, so
// that callers written against OpenSSL 3.5 interoperate while existing callers
// using the short name keep working.
TEST(ObjTest, MLDSANames) {
  const struct {
    int nid;
    const char *short_name;
    const char *long_name;
  } kMLDSA[] = {
      {NID_MLDSA44, "MLDSA44", "ML-DSA-44"},
      {NID_MLDSA65, "MLDSA65", "ML-DSA-65"},
      {NID_MLDSA87, "MLDSA87", "ML-DSA-87"},
  };
  for (const auto &t : kMLDSA) {
    SCOPED_TRACE(t.long_name);
    EXPECT_STREQ(t.short_name, OBJ_nid2sn(t.nid));
    EXPECT_STREQ(t.long_name, OBJ_nid2ln(t.nid));
    // The OpenSSL 3.5 long name resolves back to the NID.
    EXPECT_EQ(t.nid, OBJ_ln2nid(t.long_name));
    EXPECT_EQ(t.nid, OBJ_txt2nid(t.long_name));
    // The short name still resolves, for backward compatibility.
    EXPECT_EQ(t.nid, OBJ_sn2nid(t.short_name));
    EXPECT_EQ(t.nid, OBJ_txt2nid(t.short_name));
    // OBJ_obj2txt prefers the long name, so printing an ML-DSA key or
    // signature algorithm (e.g. via X509_print) shows the OpenSSL 3.5 name.
    char buf[32];
    ASSERT_GT(OBJ_obj2txt(buf, sizeof(buf), OBJ_nid2obj(t.nid), 0), 0);
    EXPECT_STREQ(t.long_name, buf);
  }
}

TEST(ObjTest, TestSignatureAlgorithms) {
  int digest_nid, pkey_nid;
  ASSERT_TRUE(OBJ_find_sigid_algs(NID_sha256WithRSAEncryption, &digest_nid,
                                  &pkey_nid));
  ASSERT_EQ(digest_nid, NID_sha256);
  ASSERT_EQ(pkey_nid, NID_rsaEncryption);

  ASSERT_FALSE(OBJ_find_sigid_algs(NID_sha256, &digest_nid, &pkey_nid));

  int sign_nid;
  ASSERT_TRUE(OBJ_find_sigid_by_algs(&sign_nid, NID_sha256, NID_rsaEncryption));
  ASSERT_EQ(sign_nid, NID_sha256WithRSAEncryption);
  ASSERT_FALSE(OBJ_find_sigid_by_algs(&sign_nid, NID_dsa, NID_rsaEncryption));
}

static bool ExpectObj2Txt(const uint8_t *der, size_t der_len,
                          bool always_return_oid, const char *expected) {
  bssl::UniquePtr<ASN1_OBJECT> obj(
      ASN1_OBJECT_create(NID_undef, der, static_cast<int>(der_len),
                         /*sn=*/nullptr, /*ln=*/nullptr));
  if (!obj) {
    return false;
  }

  int expected_len = static_cast<int>(strlen(expected));

  int len = OBJ_obj2txt(nullptr, 0, obj.get(), always_return_oid);
  if (len != expected_len) {
    fprintf(stderr,
            "OBJ_obj2txt of %s with out_len = 0 returned %d, wanted %d.\n",
            expected, len, expected_len);
    return false;
  }

  char short_buf[1];
  OPENSSL_memset(short_buf, 0xff, sizeof(short_buf));
  len = OBJ_obj2txt(short_buf, sizeof(short_buf), obj.get(), always_return_oid);
  if (len != expected_len) {
    fprintf(stderr,
            "OBJ_obj2txt of %s with out_len = 1 returned %d, wanted %d.\n",
            expected, len, expected_len);
    return false;
  }

  if (OPENSSL_memchr(short_buf, '\0', sizeof(short_buf)) == nullptr) {
    fprintf(stderr,
            "OBJ_obj2txt of %s with out_len = 1 did not NUL-terminate the "
            "output.\n",
            expected);
    return false;
  }

  char buf[256];
  len = OBJ_obj2txt(buf, sizeof(buf), obj.get(), always_return_oid);
  if (len != expected_len) {
    fprintf(stderr,
            "OBJ_obj2txt of %s with out_len = 256 returned %d, wanted %d.\n",
            expected, len, expected_len);
    return false;
  }

  if (strcmp(buf, expected) != 0) {
    fprintf(stderr, "OBJ_obj2txt returned \"%s\"; wanted \"%s\".\n", buf,
            expected);
    return false;
  }

  return true;
}

TEST(ObjTest, TestObj2Txt) {
  // kSHA256WithRSAEncryption is the DER representation of
  // 1.2.840.113549.1.1.11, id-sha256WithRSAEncryption.
  static const uint8_t kSHA256WithRSAEncryption[] = {
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b,
  };

  // kBasicConstraints is the DER representation of 2.5.29.19,
  // id-basicConstraints.
  static const uint8_t kBasicConstraints[] = {
      0x55, 0x1d, 0x13,
  };

  // kTestOID is the DER representation of 1.2.840.113554.4.1.72585.0,
  // from https://davidben.net/oid.
  static const uint8_t kTestOID[] = {
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x12, 0x04, 0x01, 0x84, 0xb7, 0x09, 0x00,
  };

  ASSERT_TRUE(
      ExpectObj2Txt(kSHA256WithRSAEncryption, sizeof(kSHA256WithRSAEncryption),
                    true /* don't return name */, "1.2.840.113549.1.1.11"));
  ASSERT_TRUE(
      ExpectObj2Txt(kSHA256WithRSAEncryption, sizeof(kSHA256WithRSAEncryption),
                    false /* return name */, "sha256WithRSAEncryption"));
  ASSERT_TRUE(ExpectObj2Txt(kBasicConstraints, sizeof(kBasicConstraints),
                            true /* don't return name */, "2.5.29.19"));
  ASSERT_TRUE(ExpectObj2Txt(kBasicConstraints, sizeof(kBasicConstraints),
                            false /* return name */,
                            "X509v3 Basic Constraints"));
  ASSERT_TRUE(ExpectObj2Txt(kTestOID, sizeof(kTestOID),
                            true /* don't return name */,
                            "1.2.840.113554.4.1.72585.0"));
  ASSERT_TRUE(ExpectObj2Txt(kTestOID, sizeof(kTestOID), false /* return name */,
                            "1.2.840.113554.4.1.72585.0"));
  // Python depends on the empty OID successfully encoding as the empty
  // string.
  ASSERT_TRUE(ExpectObj2Txt(nullptr, 0, false /* return name */, ""));
  ASSERT_TRUE(ExpectObj2Txt(nullptr, 0, true /* don't return name */, ""));

  // kNonMinimalOID is kBasicConstraints with the final component non-minimally
  // encoded.
  static const uint8_t kNonMinimalOID[] = {0x55, 0x1d, 0x80, 0x13};
  bssl::UniquePtr<ASN1_OBJECT> obj(
      ASN1_OBJECT_create(NID_undef, kNonMinimalOID, sizeof(kNonMinimalOID),
                         /*sn=*/nullptr, /*ln=*/nullptr));
  ASSERT_TRUE(obj);
  ASSERT_EQ(-1, OBJ_obj2txt(NULL, 0, obj.get(), 0));

  // kOverflowOID is the DER representation of
  // 1.2.840.113554.4.1.72585.18446744073709551616. (The final value is 2^64.)
  static const uint8_t kOverflowOID[] = {
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x12, 0x04, 0x01, 0x84, 0xb7, 0x09,
      0x82, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00,
  };
  obj.reset(ASN1_OBJECT_create(NID_undef, kOverflowOID, sizeof(kOverflowOID),
                               /*sn=*/nullptr, /*ln=*/nullptr));
  ASSERT_TRUE(obj);
  ASSERT_EQ(-1, OBJ_obj2txt(NULL, 0, obj.get(), 0));

  // kInvalidOID is a mis-encoded version of kBasicConstraints with the final
  // octet having the high bit set.
  static const uint8_t kInvalidOID[] = {0x55, 0x1d, 0x93};
  obj.reset(ASN1_OBJECT_create(NID_undef, kInvalidOID, sizeof(kInvalidOID),
                               /*sn=*/nullptr, /*ln=*/nullptr));
  ASSERT_TRUE(obj);
  ASSERT_EQ(-1, OBJ_obj2txt(NULL, 0, obj.get(), 0));
}

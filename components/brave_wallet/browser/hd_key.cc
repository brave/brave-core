/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/hd_key.h"

#include "base/check.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"
#include "brave/third_party/bitcoin-core/src/src/crypto/ripemd160.h"
#include "brave/third_party/bitcoin-core/src/src/secp256k1/include/secp256k1_recovery.h"
#include "crypto/sha2.h"
#include "third_party/boringssl/src/include/openssl/hmac.h"

namespace brave_wallet {

namespace {
constexpr char kMasterSecret[] = "Bitcoin seed";
#define SERIALIZATION_LEN 78
#define HARDENED_OFFSET 0x80000000
#define MAINNET_PUBLIC 0x0488B21E
#define MAINNET_PRIVATE 0x0488ADE4
}  // namespace

HDKey::HDKey()
    : depth_(0),
      fingerprint_(0),
      parent_fingerprint_(0),
      index_(0),
      identifier_(20),
      private_key_(0),
      public_key_(33),
      chain_code_(32),
      secp256k1_ctx_(secp256k1_context_create(SECP256K1_CONTEXT_SIGN |
                                              SECP256K1_CONTEXT_VERIFY)) {}
HDKey::HDKey(uint8_t depth, uint32_t parent_fingerprint, uint32_t index)
    : depth_(depth),
      fingerprint_(0),
      parent_fingerprint_(parent_fingerprint),
      index_(index),
      identifier_(20),
      private_key_(0),
      public_key_(33),
      chain_code_(32),
      secp256k1_ctx_(secp256k1_context_create(SECP256K1_CONTEXT_SIGN |
                                              SECP256K1_CONTEXT_VERIFY)) {}

HDKey::~HDKey() {
  secp256k1_context_destroy(secp256k1_ctx_);
  SecureZeroData(private_key_.data(), private_key_.size());
}

// static
std::unique_ptr<HDKey> HDKey::GenerateFromSeed(
    const std::vector<uint8_t>& seed) {
  // 128 - 512 bits
  if (seed.size() < 16 || seed.size() > 64) {
    LOG(ERROR) << __func__ << ": Seed size should be 16 to 64 bytes";
    return nullptr;
  }
  std::unique_ptr<HDKey> hdkey = std::make_unique<HDKey>();
  size_t hmac_length = EVP_MD_size(EVP_sha512());
  std::vector<uint8_t> hmac(hmac_length);
  unsigned int out_len;
  if (!HMAC(EVP_sha512(), kMasterSecret, sizeof(kMasterSecret), seed.data(),
            seed.size(), hmac.data(), &out_len)) {
    LOG(ERROR) << __func__ << ": HMAC_SHA512 failed";
    return nullptr;
  }
  DCHECK(out_len == hmac_length);

  const std::vector<uint8_t> IL(hmac.begin(), hmac.begin() + hmac_length / 2);
  const std::vector<uint8_t> IR(hmac.begin() + hmac_length / 2, hmac.end());
  hdkey->SetPrivateKey(IL);
  hdkey->SetChainCode(IR);
  return hdkey;
}

// static
std::unique_ptr<HDKey> HDKey::GenerateFromExtendedKey(const std::string& key) {
  std::vector<unsigned char> buf(SERIALIZATION_LEN);
  if (!DecodeBase58Check(key, buf, buf.size())) {
    LOG(ERROR) << __func__ << ": DecodeBase58Check failed";
    return nullptr;
  }

  // version(4) || depth(1) || parent_fingerprint(4) || index(4) || chain(32) ||
  // key(33)
  uint8_t* ptr = reinterpret_cast<uint8_t*>(buf.data());
  int32_t version = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3] << 0;
  DCHECK(version == MAINNET_PUBLIC || version == MAINNET_PRIVATE);
  ptr += sizeof(version);

  uint8_t depth = *ptr;
  ptr += sizeof(depth);

  int32_t parent_fingerprint =
      ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3] << 0;
  ptr += sizeof(parent_fingerprint);

  int32_t index = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3] << 0;
  ptr += sizeof(index);

  std::unique_ptr<HDKey> hdkey =
      std::make_unique<HDKey>(depth, parent_fingerprint, index);

  std::vector<uint8_t> chain_code(ptr, ptr + 32);
  ptr += chain_code.size();
  hdkey->SetChainCode(chain_code);

  if (*ptr == 0x00) {
    DCHECK_EQ(version, MAINNET_PRIVATE);
    std::vector<uint8_t> key(ptr + 1, ptr + 33);
    hdkey->SetPrivateKey(key);
  } else {
    DCHECK_EQ(version, MAINNET_PUBLIC);
    std::vector<uint8_t> key(ptr, ptr + 33);
    hdkey->SetPublicKey(key);
  }

  return hdkey;
}

void HDKey::SetPrivateKey(const std::vector<uint8_t>& value) {
  if (value.size() != 32) {
    LOG(ERROR) << __func__ << ": pivate key must be 32 bytes";
    return;
  }
  private_key_ = value;
  GeneratePublicKey();
  identifier_ = Hash160(public_key_);

  const uint8_t* ptr = identifier_.data();
  fingerprint_ = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3] << 0;
}

std::string HDKey::GetPrivateExtendedKey() const {
  std::vector<uint8_t> key;
  key.push_back(0x00);
  key.insert(key.end(), private_key_.begin(), private_key_.end());
  return Serialize(MAINNET_PRIVATE, key);
}

void HDKey::SetPublicKey(const std::vector<uint8_t>& value) {
  // Compressed only
  if (value.size() != 33) {
    LOG(ERROR) << __func__ << ": public key must be 33 bytes";
    return;
  }
  // Verify public key
  secp256k1_pubkey pubkey;
  if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_, &pubkey, value.data(),
                                 value.size())) {
    LOG(ERROR) << __func__ << ": not a valid public key";
    return;
  }
  public_key_ = value;
  identifier_ = Hash160(public_key_);

  const uint8_t* ptr = identifier_.data();
  fingerprint_ = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3] << 0;
}

std::string HDKey::GetPublicExtendedKey() const {
  return Serialize(MAINNET_PUBLIC, public_key_);
}

std::vector<uint8_t> HDKey::GetUncompressedPublicKey() const {
  // uncompressed
  size_t public_key_len = 65;
  std::vector<uint8_t> public_key(public_key_len);
  secp256k1_pubkey pubkey;
  if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_, &pubkey, public_key_.data(),
                                 public_key_.size())) {
    LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_parse failed";
    return public_key;
  }
  if (!secp256k1_ec_pubkey_serialize(secp256k1_ctx_, public_key.data(),
                                     &public_key_len, &pubkey,
                                     SECP256K1_EC_UNCOMPRESSED)) {
    LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_serialize failed";
  }

  return public_key;
}

void HDKey::SetChainCode(const std::vector<uint8_t>& value) {
  chain_code_ = value;
}

std::unique_ptr<HDKey> HDKey::DeriveChild(uint32_t index) {
  std::unique_ptr<HDKey> hdkey = std::make_unique<HDKey>();
  bool is_hardened = index >= HARDENED_OFFSET;
  std::vector<uint8_t> data;

  if (is_hardened) {
    // Hardened: data = 0x00 || ser256(kpar) || ser32(index)
    DCHECK(!private_key_.empty());
    data.push_back(0x00);
    data.insert(data.end(), private_key_.begin(), private_key_.end());
  } else {
    // Normal private: data = serP(point(kpar)) || ser32(index)
    // Normal pubic  : data = serP(Kpar) || ser32(index)
    //     serP(Kpar) is public key when point(kpar) is private key
    data.insert(data.end(), public_key_.begin(), public_key_.end());
  }
  data.push_back((index >> 24) & 0xFF);
  data.push_back((index >> 16) & 0xFF);
  data.push_back((index >> 8) & 0xFF);
  data.push_back(index & 0xFF);

  size_t hmac_length = EVP_MD_size(EVP_sha512());
  std::vector<uint8_t> hmac(hmac_length);
  unsigned int out_len;
  if (!HMAC(EVP_sha512(), chain_code_.data(), chain_code_.size(), data.data(),
            data.size(), hmac.data(), &out_len)) {
    LOG(ERROR) << __func__ << ": HMAC_SHA512 failed";
    return nullptr;
  }
  DCHECK(out_len == hmac_length);

  const std::vector<uint8_t> IL(hmac.begin(), hmac.begin() + hmac_length / 2);
  const std::vector<uint8_t> IR(hmac.begin() + hmac_length / 2, hmac.end());

  hdkey->chain_code_ = IR;

  if (!private_key_.empty()) {
    // Private parent key -> private child key
    // Also Private parent key -> public child key because we always create
    // public key.
    std::vector<uint8_t> private_key = private_key_;
    if (!secp256k1_ec_seckey_tweak_add(secp256k1_ctx_, private_key.data(),
                                       IL.data())) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_seckey_tweak_add failed";
      return nullptr;
    }
    hdkey->SetPrivateKey(private_key);
  } else {
    // Public parent key -> public child key (Normal only)
    DCHECK(!is_hardened);
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_, &pubkey, public_key_.data(),
                                   public_key_.size())) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_parse failed";
      return nullptr;
    }

    if (!secp256k1_ec_pubkey_tweak_add(secp256k1_ctx_, &pubkey, IL.data())) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_tweak_add failed";
      return nullptr;
    }
    size_t public_key_len = 33;
    std::vector<uint8_t> public_key(public_key_len);
    if (!secp256k1_ec_pubkey_serialize(secp256k1_ctx_, public_key.data(),
                                       &public_key_len, &pubkey,
                                       SECP256K1_EC_COMPRESSED)) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_serialize failed";
      return nullptr;
    }
    hdkey->SetPublicKey(public_key);
  }

  hdkey->depth_ = depth_ + 1;
  hdkey->parent_fingerprint_ = fingerprint_;
  hdkey->index_ = index;

  return hdkey;
}

std::unique_ptr<HDKey> HDKey::DeriveChildFromPath(const std::string& path) {
  std::unique_ptr<HDKey> hd_key = std::make_unique<HDKey>();
  if (path == "m") {
    if (!private_key_.empty())
      hd_key->SetPrivateKey(private_key_);
    else
      hd_key->SetPublicKey(public_key_);
    hd_key->chain_code_ = chain_code_;
    return hd_key;
  }
  std::vector<std::string> entries =
      base::SplitString(path, "/", base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);
  if (entries.empty())
    return nullptr;
  for (size_t i = 0; i < entries.size(); ++i) {
    std::string entry = entries[i];
    if (i == 0) {
      if (entry != "m") {
        LOG(ERROR) << __func__ << ": path must starts with \"m\"";
        return nullptr;
      }
      if (!private_key_.empty())
        hd_key->SetPrivateKey(private_key_);
      else
        hd_key->SetPublicKey(public_key_);
      hd_key->chain_code_ = chain_code_;
      continue;
    }
    bool is_hardened = entry.length() > 1 && entry.back() == '\'';
    if (is_hardened)
      entry.pop_back();
    unsigned child_index = 0;
    if (!base::StringToUint(entry, &child_index)) {
      LOG(ERROR) << __func__ << ": path must contains number or number'";
      return nullptr;
    }
    if (child_index >= HARDENED_OFFSET) {
      LOG(ERROR) << __func__ << ": index must be less than " << HARDENED_OFFSET;
      return nullptr;
    }
    if (is_hardened)
      child_index += HARDENED_OFFSET;

    hd_key = hd_key->DeriveChild(child_index);
    if (!hd_key)
      return nullptr;
  }
  return hd_key;
}

std::vector<uint8_t> HDKey::Sign(const std::vector<uint8_t>& msg, int* recid) {
  std::vector<uint8_t> sig(64);
  if (msg.size() != 32) {
    LOG(ERROR) << __func__ << ": message length should be 32";
    return sig;
  }
  if (!recid) {
    secp256k1_ecdsa_signature ecdsa_sig;
    if (!secp256k1_ecdsa_sign(secp256k1_ctx_, &ecdsa_sig, msg.data(),
                              private_key_.data(),
                              secp256k1_nonce_function_rfc6979, nullptr)) {
      LOG(ERROR) << __func__ << ": secp256k1_ecdsa_sign failed";
      return sig;
    }

    if (!secp256k1_ecdsa_signature_serialize_compact(secp256k1_ctx_, sig.data(),
                                                     &ecdsa_sig)) {
      LOG(ERROR) << __func__
                 << ": secp256k1_ecdsa_signature_serialize_compact failed";
    }
  } else {
    secp256k1_ecdsa_recoverable_signature ecdsa_sig;
    if (!secp256k1_ecdsa_sign_recoverable(
            secp256k1_ctx_, &ecdsa_sig, msg.data(), private_key_.data(),
            secp256k1_nonce_function_rfc6979, nullptr)) {
      LOG(ERROR) << __func__ << ": secp256k1_ecdsa_sign_recoverable failed";
      return sig;
    }
    if (!secp256k1_ecdsa_recoverable_signature_serialize_compact(
            secp256k1_ctx_, sig.data(), recid, &ecdsa_sig)) {
      LOG(ERROR)
          << __func__
          << ": secp256k1_ecdsa_recoverable_signature_serialize_compact failed";
    }
  }

  return sig;
}

bool HDKey::Verify(const std::vector<uint8_t>& msg,
                   const std::vector<uint8_t>& sig) {
  if (msg.size() != 32 || sig.size() != 64) {
    LOG(ERROR) << __func__ << ": message or signature length is invalid";
    return false;
  }

  secp256k1_ecdsa_signature ecdsa_sig;
  if (!secp256k1_ecdsa_signature_parse_compact(secp256k1_ctx_, &ecdsa_sig,
                                               sig.data())) {
    LOG(ERROR) << __func__
               << ": secp256k1_ecdsa_signature_parse_compact failed";
    return false;
  }
  secp256k1_pubkey pubkey;
  if (!secp256k1_ec_pubkey_parse(secp256k1_ctx_, &pubkey, public_key_.data(),
                                 public_key_.size())) {
    LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_parse failed";
    return false;
  }
  if (!secp256k1_ecdsa_verify(secp256k1_ctx_, &ecdsa_sig, msg.data(),
                              &pubkey)) {
    LOG(ERROR) << __func__ << ": secp256k1_ecdsa_verify failed";
    return false;
  }
  return true;
}

std::vector<uint8_t> HDKey::Recover(const std::vector<uint8_t>& msg,
                                    const std::vector<uint8_t>& sig,
                                    int recid) {
  size_t public_key_len = 33;
  std::vector<uint8_t> public_key(public_key_len);
  if (msg.size() != 32 || sig.size() != 64) {
    LOG(ERROR) << __func__ << ": message or signature length is invalid";
    return public_key;
  }
  if (recid < 0 || recid > 3) {
    LOG(ERROR) << __func__ << ": recovery id must be 0, 1, 2 or 3";
    return public_key;
  }

  secp256k1_ecdsa_recoverable_signature ecdsa_sig;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
          secp256k1_ctx_, &ecdsa_sig, sig.data(), recid)) {
    LOG(ERROR)
        << __func__
        << ": secp256k1_ecdsa_recoverable_signature_parse_compact failed";
    return public_key;
  }

  secp256k1_pubkey pubkey;
  if (!secp256k1_ecdsa_recover(secp256k1_ctx_, &pubkey, &ecdsa_sig,
                               msg.data())) {
    LOG(ERROR) << __func__ << ": secp256k1_ecdsa_recover failed";
    return public_key;
  }

  if (!secp256k1_ec_pubkey_serialize(secp256k1_ctx_, public_key.data(),
                                     &public_key_len, &pubkey,
                                     SECP256K1_EC_COMPRESSED)) {
    LOG(ERROR) << "secp256k1_ec_pubkey_serialize failed";
  }

  return public_key;
}

void HDKey::GeneratePublicKey() {
  secp256k1_pubkey public_key;
  if (!secp256k1_ec_pubkey_create(secp256k1_ctx_, &public_key,
                                  private_key_.data())) {
    LOG(ERROR) << "secp256k1_ec_pubkey_create failed";
    return;
  }
  size_t public_key_len = 33;
  if (!secp256k1_ec_pubkey_serialize(secp256k1_ctx_, public_key_.data(),
                                     &public_key_len, &public_key,
                                     SECP256K1_EC_COMPRESSED)) {
    LOG(ERROR) << "secp256k1_ec_pubkey_serialize failed";
  }
}

std::string HDKey::Serialize(uint32_t version,
                             const std::vector<uint8_t>& key) const {
  // version(4) || depth(1) || parent_fingerprint(4) || index(4) || chain(32) ||
  // key(33)
  std::vector<uint8_t> buf;

  buf.push_back((version >> 24) & 0xFF);
  buf.push_back((version >> 16) & 0xFF);
  buf.push_back((version >> 8) & 0xFF);
  buf.push_back((version >> 0) & 0xFF);

  buf.push_back(depth_);

  buf.push_back((parent_fingerprint_ >> 24) & 0xFF);
  buf.push_back((parent_fingerprint_ >> 16) & 0xFF);
  buf.push_back((parent_fingerprint_ >> 8) & 0xFF);
  buf.push_back(parent_fingerprint_ & 0xFF);

  buf.push_back((index_ >> 24) & 0xFF);
  buf.push_back((index_ >> 16) & 0xFF);
  buf.push_back((index_ >> 8) & 0xFF);
  buf.push_back(index_ & 0xFF);

  buf.insert(buf.end(), chain_code_.begin(), chain_code_.end());
  DCHECK_EQ(key.size(), 33u);
  buf.insert(buf.end(), key.begin(), key.end());

  DCHECK(buf.size() == SERIALIZATION_LEN);
  return EncodeBase58Check(buf);
}

const std::vector<uint8_t> HDKey::Hash160(const std::vector<uint8_t>& input) {
  // BoringSSL in chromium doesn't have ripemd implementation built in BUILD.gn
  // only header
  std::vector<uint8_t> result(CRIPEMD160::OUTPUT_SIZE);

  std::array<uint8_t, crypto::kSHA256Length> sha256hash =
      crypto::SHA256Hash(input);
  DCHECK(!sha256hash.empty());

  CRIPEMD160()
      .Write(sha256hash.data(), sha256hash.size())
      .Finalize(result.data());

  return result;
}

}  // namespace brave_wallet

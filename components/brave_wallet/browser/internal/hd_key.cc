/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/containers/span_writer.h"
#include "base/containers/to_vector.h"
#include "base/logging.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"
#include "brave/vendor/bat-native-tweetnacl/tweetnacl.h"
#include "crypto/process_bound_string.h"
#include "crypto/random.h"
#include "third_party/boringssl/src/include/openssl/hmac.h"

#define SECP256K1_BUILD  // This effectively turns off export attributes.
#include "brave/third_party/bitcoin-core/src/src/secp256k1/include/secp256k1.h"
#include "brave/third_party/bitcoin-core/src/src/secp256k1/include/secp256k1_recovery.h"

namespace brave_wallet {

namespace {
constexpr char kMasterSecret[] = "Bitcoin seed";
constexpr size_t kSHA512Length = 64;
constexpr size_t kSerializationLength = 78;
constexpr size_t kMaxDerSignatureSize = 72;
constexpr size_t kContextRandomizeSize = 32;

const secp256k1_context* GetSecp256k1Ctx() {
  static const secp256k1_context* const kSecp256k1Ctx = [] {
    secp256k1_context* context = secp256k1_context_create(
        SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    SecureVector bytes(kContextRandomizeSize);
    crypto::RandBytes(bytes);
    [[maybe_unused]] int result =
        secp256k1_context_randomize(context, bytes.data());

    return context;
  }();

  return kSecp256k1Ctx;
}

}  // namespace

HDKey::HDKey() : public_key_(33), chain_code_(32) {}
HDKey::~HDKey() = default;
HDKey& HDKey::operator=(const HDKey& other) = default;
HDKey::HDKey(const HDKey& other) = default;

HDKey::ParsedExtendedKey::ParsedExtendedKey() = default;
HDKey::ParsedExtendedKey::~ParsedExtendedKey() = default;

// static
std::unique_ptr<HDKey> HDKey::GenerateFromSeed(base::span<const uint8_t> seed) {
  // 128 - 512 bits
  if (seed.size() < 16 || seed.size() > 64) {
    LOG(ERROR) << __func__ << ": Seed size should be 16 to 64 bytes";
    return nullptr;
  }

  SecureVector hmac(kSHA512Length);
  unsigned int out_len;
  if (!HMAC(EVP_sha512(), kMasterSecret, sizeof(kMasterSecret), seed.data(),
            seed.size(), hmac.data(), &out_len)) {
    LOG(ERROR) << __func__ << ": HMAC_SHA512 failed";
    return nullptr;
  }
  DCHECK(out_len == kSHA512Length);

  std::unique_ptr<HDKey> hdkey = std::make_unique<HDKey>();
  auto hmac_span = base::span(hmac);
  hdkey->SetPrivateKey(hmac_span.first<kSecp256k1PrivateKeySize>());
  hdkey->SetChainCode(hmac_span.last<kSecp256k1ChainCodeSize>());
  return hdkey;
}

// static
std::unique_ptr<HDKey::ParsedExtendedKey> HDKey::GenerateFromExtendedKey(
    const std::string& key) {
  std::vector<unsigned char> decoded_key(kSerializationLength);
  if (!DecodeBase58Check(key, decoded_key, decoded_key.size())) {
    LOG(ERROR) << __func__ << ": DecodeBase58Check failed";
    return nullptr;
  }

  SecureVector buf(decoded_key.begin(), decoded_key.end());
  crypto::internal::SecureZeroBuffer(base::as_writable_byte_span(decoded_key));

  // version(4) || depth(1) || parent_fingerprint(4) || index(4) || chain(32) ||
  // key(33)
  auto reader = base::SpanReader(base::as_byte_span(buf));

  auto result = std::make_unique<ParsedExtendedKey>();
  result->hdkey = std::make_unique<HDKey>();

  reader.ReadU32BigEndian(result->version);
  reader.ReadU8BigEndian(result->hdkey->depth_);
  reader.ReadCopy(result->hdkey->parent_fingerprint_);
  reader.ReadU32BigEndian(result->hdkey->index_);
  reader.ReadCopy(result->hdkey->chain_code_);

  auto key_bytes = reader.Read<kSecp256k1PubkeySize>();
  CHECK(key_bytes);
  if (key_bytes->front() == 0x00) {
    // Skip first zero byte which is not part of private key.
    result->hdkey->SetPrivateKey(key_bytes->subspan<1>());
  } else {
    result->hdkey->SetPublicKey(*key_bytes);
  }
  DCHECK_EQ(reader.remaining(), 0u);

  return result;
}

// static
std::unique_ptr<HDKey> HDKey::GenerateFromPrivateKey(
    base::span<const uint8_t, kSecp256k1PrivateKeySize> private_key) {
  std::unique_ptr<HDKey> hd_key = std::make_unique<HDKey>();
  hd_key->SetPrivateKey(private_key);
  return hd_key;
}

void HDKey::SetPrivateKey(
    base::span<const uint8_t, kSecp256k1PrivateKeySize> value) {
  private_key_.assign(value.begin(), value.end());
  GeneratePublicKey();
}

std::string HDKey::GetPrivateExtendedKey(ExtendedKeyVersion version) const {
  return Serialize(version, private_key_);
}

std::vector<uint8_t> HDKey::GetPrivateKeyBytes() const {
  return std::vector<uint8_t>(private_key_.begin(), private_key_.end());
}

std::vector<uint8_t> HDKey::GetPublicKeyBytes() const {
  DCHECK(!public_key_.empty());
  return public_key_;
}

void HDKey::SetPublicKey(
    base::span<const uint8_t, kSecp256k1PubkeySize> value) {
  // Verify public key
  secp256k1_pubkey pubkey;
  if (!secp256k1_ec_pubkey_parse(GetSecp256k1Ctx(), &pubkey, value.data(),
                                 value.size())) {
    LOG(ERROR) << __func__ << ": not a valid public key";
    return;
  }
  public_key_ = base::ToVector(value);
}

std::string HDKey::GetPublicExtendedKey(ExtendedKeyVersion version) const {
  return Serialize(version, public_key_);
}

std::vector<uint8_t> HDKey::GetUncompressedPublicKey() const {
  // uncompressed
  size_t public_key_len = 65;
  std::vector<uint8_t> public_key(public_key_len);
  secp256k1_pubkey pubkey;
  if (!secp256k1_ec_pubkey_parse(GetSecp256k1Ctx(), &pubkey, public_key_.data(),
                                 public_key_.size())) {
    LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_parse failed";
    return public_key;
  }
  if (!secp256k1_ec_pubkey_serialize(GetSecp256k1Ctx(), public_key.data(),
                                     &public_key_len, &pubkey,
                                     SECP256K1_EC_UNCOMPRESSED)) {
    LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_serialize failed";
  }

  return public_key;
}

std::string HDKey::GetZCashTransparentAddress(bool testnet) const {
  return PubkeyToTransparentAddress(public_key_, testnet);
}

std::vector<uint8_t> HDKey::GetPublicKeyFromX25519_XSalsa20_Poly1305() const {
  size_t public_key_len = crypto_scalarmult_curve25519_tweet_BYTES;
  std::vector<uint8_t> public_key(public_key_len);
  const uint8_t* private_key_ptr = private_key_.data();
  if (crypto_scalarmult_curve25519_tweet_base(public_key.data(),
                                              private_key_ptr) != 0) {
    return std::vector<uint8_t>();
  }
  return public_key;
}

std::optional<std::vector<uint8_t>>
HDKey::DecryptCipherFromX25519_XSalsa20_Poly1305(
    const std::string& version,
    base::span<const uint8_t> nonce,
    base::span<const uint8_t> ephemeral_public_key,
    base::span<const uint8_t> ciphertext) const {
  // Only x25519-xsalsa20-poly1305 is supported by MM at the time of writing
  if (version != "x25519-xsalsa20-poly1305") {
    return std::nullopt;
  }
  if (nonce.size() != crypto_box_curve25519xsalsa20poly1305_tweet_NONCEBYTES) {
    return std::nullopt;
  }
  if (ephemeral_public_key.size() !=
      crypto_box_curve25519xsalsa20poly1305_tweet_PUBLICKEYBYTES) {
    return std::nullopt;
  }
  if (private_key_.size() !=
      crypto_box_curve25519xsalsa20poly1305_tweet_SECRETKEYBYTES) {
    return std::nullopt;
  }

  std::vector<uint8_t> padded_ciphertext(ciphertext.begin(), ciphertext.end());
  padded_ciphertext.insert(padded_ciphertext.begin(), crypto_box_BOXZEROBYTES,
                           0);
  std::vector<uint8_t> padded_plaintext(padded_ciphertext.size());
  const uint8_t* private_key_ptr = private_key_.data();
  if (crypto_box_open(padded_plaintext.data(), padded_ciphertext.data(),
                      padded_ciphertext.size(), nonce.data(),
                      ephemeral_public_key.data(), private_key_ptr) != 0) {
    return std::nullopt;
  }
  std::vector<uint8_t> plaintext(
      padded_plaintext.cbegin() + crypto_box_ZEROBYTES,
      padded_plaintext.cend());
  return plaintext;
}

void HDKey::SetChainCode(
    base::span<const uint8_t, kSecp256k1ChainCodeSize> value) {
  chain_code_.assign(value.begin(), value.end());
}

std::unique_ptr<HDKey> HDKey::DeriveChild(const DerivationIndex& index) {
  auto index_value = index.GetValue();
  if (!index_value) {
    return nullptr;
  }

  SecureVector data(kSecp256k1PubkeySize + sizeof(uint32_t));
  auto span_writer = base::SpanWriter(base::span(data));

  if (index.is_hardened()) {
    // Hardened: data = 0x00 || ser256(kpar) || ser32(index)
    DCHECK(!private_key_.empty());
    span_writer.WriteI8BigEndian(0x00);
    span_writer.Write(private_key_);
  } else {
    // Normal private: data = serP(point(kpar)) || ser32(index)
    // Normal pubic  : data = serP(Kpar) || ser32(index)
    //     serP(Kpar) is public key when point(kpar) is private key
    span_writer.Write(public_key_);
  }
  span_writer.WriteU32BigEndian(*index_value);
  DCHECK_EQ(span_writer.remaining(), 0u);

  SecureVector hmac(kSHA512Length);
  unsigned int out_len;
  if (!HMAC(EVP_sha512(), chain_code_.data(), chain_code_.size(), data.data(),
            data.size(), hmac.data(), &out_len)) {
    LOG(ERROR) << __func__ << ": HMAC_SHA512 failed";
    return nullptr;
  }
  DCHECK(out_len == kSHA512Length);

  auto hmac_span = base::span(hmac);

  std::unique_ptr<HDKey> hdkey = std::make_unique<HDKey>();
  hdkey->SetChainCode(hmac_span.last<kSecp256k1ChainCodeSize>());

  if (!private_key_.empty()) {
    // Private parent key -> private child key
    // Also Private parent key -> public child key because we always create
    // public key.
    SecureVector private_key = private_key_;
    if (!secp256k1_ec_seckey_tweak_add(GetSecp256k1Ctx(), private_key.data(),
                                       hmac_span.first<32>().data())) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_seckey_tweak_add failed";
      return nullptr;
    }
    hdkey->SetPrivateKey(
        *base::span(private_key).to_fixed_extent<kSecp256k1PrivateKeySize>());
  } else {
    // Public parent key -> public child key (Normal only)
    DCHECK(!index.is_hardened());
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(GetSecp256k1Ctx(), &pubkey,
                                   public_key_.data(), public_key_.size())) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_parse failed";
      return nullptr;
    }

    if (!secp256k1_ec_pubkey_tweak_add(GetSecp256k1Ctx(), &pubkey,
                                       hmac_span.first<32>().data())) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_tweak_add failed";
      return nullptr;
    }
    size_t public_key_len = kSecp256k1PubkeySize;
    std::array<uint8_t, kSecp256k1PubkeySize> public_key;
    if (!secp256k1_ec_pubkey_serialize(GetSecp256k1Ctx(), public_key.data(),
                                       &public_key_len, &pubkey,
                                       SECP256K1_EC_COMPRESSED)) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_serialize failed";
      return nullptr;
    }
    hdkey->SetPublicKey(public_key);
  }

  hdkey->depth_ = depth_ + 1;
  hdkey->parent_fingerprint_ = GetFingerprint();
  hdkey->index_ = *index_value;

  return hdkey;
}

std::unique_ptr<HDKey> HDKey::DeriveChildFromPath(
    base::span<const DerivationIndex> path) {
  auto hd_key = base::WrapUnique(new HDKey(*this));

  for (auto index : path) {
    hd_key = hd_key->DeriveChild(index);
    if (!hd_key) {
      return nullptr;
    }
  }

  return hd_key;
}

std::optional<std::array<uint8_t, kCompactSignatureSize>> HDKey::SignCompact(
    Secp256k1SignMsgSpan msg,
    int* recid) {
  std::array<uint8_t, kCompactSignatureSize> sig = {};
  if (!recid) {
    secp256k1_ecdsa_signature ecdsa_sig;
    if (!secp256k1_ecdsa_sign(GetSecp256k1Ctx(), &ecdsa_sig, msg.data(),
                              private_key_.data(),
                              secp256k1_nonce_function_rfc6979, nullptr)) {
      return std::nullopt;
    }

    if (!secp256k1_ecdsa_signature_serialize_compact(GetSecp256k1Ctx(),
                                                     sig.data(), &ecdsa_sig)) {
      return std::nullopt;
    }
  } else {
    secp256k1_ecdsa_recoverable_signature ecdsa_sig;
    if (!secp256k1_ecdsa_sign_recoverable(
            GetSecp256k1Ctx(), &ecdsa_sig, msg.data(), private_key_.data(),
            secp256k1_nonce_function_rfc6979, nullptr)) {
      return std::nullopt;
    }
    if (!secp256k1_ecdsa_recoverable_signature_serialize_compact(
            GetSecp256k1Ctx(), sig.data(), recid, &ecdsa_sig)) {
      return std::nullopt;
    }
  }

  return sig;
}

std::optional<std::vector<uint8_t>> HDKey::SignDer(Secp256k1SignMsgSpan msg) {
  secp256k1_ecdsa_signature ecdsa_sig;
  if (!secp256k1_ecdsa_sign(GetSecp256k1Ctx(), &ecdsa_sig, msg.data(),
                            private_key_.data(),
                            secp256k1_nonce_function_rfc6979, nullptr)) {
    return std::nullopt;
  }

  auto sig_has_low_r = [](const secp256k1_context* ctx,
                          const secp256k1_ecdsa_signature* sig) {
    uint8_t compact_sig[kCompactSignatureSize] = {};
    secp256k1_ecdsa_signature_serialize_compact(ctx, compact_sig, sig);

    return compact_sig[0] < 0x80;
  };

  // Grind R https://github.com/bitcoin/bitcoin/pull/13666
  unsigned char extra_entropy[32] = {0};
  uint32_t extra_entropy_counter = 0;
  while (!sig_has_low_r(GetSecp256k1Ctx(), &ecdsa_sig)) {
    base::as_writable_byte_span(extra_entropy)
        .first<4>()
        .copy_from(base::byte_span_from_ref(base::numerics::U32FromLittleEndian(
            base::byte_span_from_ref(++extra_entropy_counter))));

    if (!secp256k1_ecdsa_sign(
            GetSecp256k1Ctx(), &ecdsa_sig, msg.data(), private_key_.data(),
            secp256k1_nonce_function_rfc6979, extra_entropy)) {
      LOG(ERROR) << __func__ << ": secp256k1_ecdsa_sign failed";
      return std::nullopt;
    }
  }

  std::vector<uint8_t> sig_der(kMaxDerSignatureSize);
  size_t sig_der_length = sig_der.size();
  if (!secp256k1_ecdsa_signature_serialize_der(
          GetSecp256k1Ctx(), sig_der.data(), &sig_der_length, &ecdsa_sig)) {
    return std::nullopt;
  }
  sig_der.resize(sig_der_length);

  // TODO(apaymyshev): verify signature?

  return sig_der;
}

bool HDKey::VerifyForTesting(Secp256k1SignMsgSpan msg,
                             CompactSignatureSpan sig) {
  secp256k1_ecdsa_signature ecdsa_sig;
  if (!secp256k1_ecdsa_signature_parse_compact(GetSecp256k1Ctx(), &ecdsa_sig,
                                               sig.data())) {
    LOG(ERROR) << __func__
               << ": secp256k1_ecdsa_signature_parse_compact failed";
    return false;
  }
  secp256k1_pubkey pubkey;
  if (!secp256k1_ec_pubkey_parse(GetSecp256k1Ctx(), &pubkey, public_key_.data(),
                                 public_key_.size())) {
    LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_parse failed";
    return false;
  }
  if (!secp256k1_ecdsa_verify(GetSecp256k1Ctx(), &ecdsa_sig, msg.data(),
                              &pubkey)) {
    LOG(ERROR) << __func__ << ": secp256k1_ecdsa_verify failed";
    return false;
  }
  return true;
}

std::array<uint8_t, kSecp256k1IdentifierSize> HDKey::GetIdentifier() const {
  return Hash160(public_key_);
}

std::array<uint8_t, kSecp256k1FingerprintSize> HDKey::GetFingerprint() const {
  auto identifier = GetIdentifier();
  std::array<uint8_t, kSecp256k1FingerprintSize> result;
  base::span(result).copy_from(
      base::span(identifier).first<kSecp256k1FingerprintSize>());
  return result;
}

std::vector<uint8_t> HDKey::RecoverCompact(bool compressed,
                                           Secp256k1SignMsgSpan msg,
                                           CompactSignatureSpan sig,
                                           int recid) {
  size_t public_key_len = compressed ? 33 : 65;
  std::vector<uint8_t> public_key(public_key_len);
  if (msg.size() != 32 || sig.size() != kCompactSignatureSize) {
    LOG(ERROR) << __func__ << ": message or signature length is invalid";
    return public_key;
  }
  if (recid < 0 || recid > 3) {
    LOG(ERROR) << __func__ << ": recovery id must be 0, 1, 2 or 3";
    return public_key;
  }

  secp256k1_ecdsa_recoverable_signature ecdsa_sig;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
          GetSecp256k1Ctx(), &ecdsa_sig, sig.data(), recid)) {
    LOG(ERROR)
        << __func__
        << ": secp256k1_ecdsa_recoverable_signature_parse_compact failed";
    return public_key;
  }

  secp256k1_pubkey pubkey;
  if (!secp256k1_ecdsa_recover(GetSecp256k1Ctx(), &pubkey, &ecdsa_sig,
                               msg.data())) {
    LOG(ERROR) << __func__ << ": secp256k1_ecdsa_recover failed";
    return public_key;
  }

  if (!secp256k1_ec_pubkey_serialize(
          GetSecp256k1Ctx(), public_key.data(), &public_key_len, &pubkey,
          compressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED)) {
    LOG(ERROR) << "secp256k1_ec_pubkey_serialize failed";
  }

  return public_key;
}

void HDKey::GeneratePublicKey() {
  secp256k1_pubkey public_key;
  if (!secp256k1_ec_pubkey_create(GetSecp256k1Ctx(), &public_key,
                                  private_key_.data())) {
    LOG(ERROR) << "secp256k1_ec_pubkey_create failed";
    return;
  }
  size_t public_key_len = 33;
  if (!secp256k1_ec_pubkey_serialize(GetSecp256k1Ctx(), public_key_.data(),
                                     &public_key_len, &public_key,
                                     SECP256K1_EC_COMPRESSED)) {
    LOG(ERROR) << "secp256k1_ec_pubkey_serialize failed";
  }
}

// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#serialization-format
std::string HDKey::Serialize(ExtendedKeyVersion version,
                             base::span<const uint8_t> key) const {
  // version(4) || depth(1) || parent_fingerprint(4) || index(4) || chain(32) ||
  // key(32 or 33)
  SecureVector buf(kSerializationLength);
  auto span_writer = base::SpanWriter(base::span(buf));
  span_writer.WriteU32BigEndian(static_cast<uint32_t>(version));
  span_writer.WriteU8BigEndian(depth_);
  span_writer.Write(parent_fingerprint_);
  span_writer.WriteU32BigEndian(index_);
  span_writer.Write(chain_code_);

  if (key.size() == 32) {
    DCHECK(version == ExtendedKeyVersion::kXprv ||
           version == ExtendedKeyVersion::kYprv ||
           version == ExtendedKeyVersion::kZprv);
    // 32-bytes private key is padded with a zero byte.
    span_writer.WriteU8BigEndian(0);
  } else {
    DCHECK(version == ExtendedKeyVersion::kXpub ||
           version == ExtendedKeyVersion::kYpub ||
           version == ExtendedKeyVersion::kZpub);
    DCHECK_EQ(key.size(), 33u);
  }
  span_writer.Write(key);

  DCHECK_EQ(span_writer.remaining(), 0u);
  return EncodeBase58Check(buf);
}

}  // namespace brave_wallet

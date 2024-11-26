/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_wallet/browser/internal/hd_key.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/containers/to_vector.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_utils.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"
#include "brave/vendor/bat-native-tweetnacl/tweetnacl.h"
#include "crypto/encryptor.h"
#include "crypto/kdf.h"
#include "crypto/process_bound_string.h"
#include "crypto/random.h"
#include "crypto/symmetric_key.h"

#define SECP256K1_BUILD  // This effectively turns off export attributes.
#include "brave/third_party/bitcoin-core/src/src/secp256k1/include/secp256k1.h"
#include "brave/third_party/bitcoin-core/src/src/secp256k1/include/secp256k1_recovery.h"

using crypto::Encryptor;
using crypto::SymmetricKey;

namespace brave_wallet {

namespace {
constexpr char kMasterSecret[] = "Bitcoin seed";
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

bool UTCPasswordVerification(const std::string& derived_key,
                             base::span<const uint8_t> ciphertext,
                             const std::string& mac,
                             size_t dklen) {
  std::vector<uint8_t> mac_verification_input(derived_key.end() - dklen / 2,
                                              derived_key.end());
  mac_verification_input.insert(mac_verification_input.end(),
                                ciphertext.begin(), ciphertext.end());
  // verify password
  if (HexEncodeLower(KeccakHash(mac_verification_input)) != mac) {
    VLOG(0) << __func__ << ": password does not match";
    return false;
  }
  return true;
}

bool UTCDecryptPrivateKey(const std::string& derived_key,
                          base::span<const uint8_t> ciphertext,
                          base::span<const uint8_t> iv,
                          std::vector<uint8_t>* private_key,
                          size_t dklen) {
  if (!private_key) {
    return false;
  }
  std::unique_ptr<SymmetricKey> decryption_key =
      SymmetricKey::Import(SymmetricKey::AES, derived_key.substr(0, dklen / 2));
  if (!decryption_key) {
    VLOG(1) << __func__ << ": raw key has to be 16 or 32 bytes for AES import";
    return false;
  }
  Encryptor encryptor;
  if (!encryptor.Init(decryption_key.get(), Encryptor::Mode::CTR,
                      std::vector<uint8_t>())) {
    VLOG(0) << __func__ << ": encryptor init failed";
    return false;
  }
  if (!encryptor.SetCounter(iv)) {
    VLOG(0) << __func__ << ": encryptor set counter failed";
    return false;
  }

  if (!encryptor.Decrypt(ciphertext, private_key)) {
    VLOG(0) << __func__ << ": encryptor decrypt failed";
    return false;
  }

  return true;
}

}  // namespace

HDKey::HDKey() : identifier_(20), public_key_(33), chain_code_(32) {}
HDKey::~HDKey() = default;

HDKey::ParsedExtendedKey::ParsedExtendedKey() = default;
HDKey::ParsedExtendedKey::~ParsedExtendedKey() = default;

// static
std::unique_ptr<HDKey> HDKey::GenerateFromSeed(base::span<const uint8_t> seed) {
  // 128 - 512 bits
  if (seed.size() < 16 || seed.size() > 64) {
    LOG(ERROR) << __func__ << ": Seed size should be 16 to 64 bytes";
    return nullptr;
  }

  auto hmac = HmacSha512(base::byte_span_from_cstring(kMasterSecret), seed);
  auto [IL, IR] = base::span(hmac).split_at(kSHA512HashLength / 2);

  std::unique_ptr<HDKey> hdkey = std::make_unique<HDKey>();
  hdkey->SetPrivateKey(IL);
  hdkey->SetChainCode(IR);
  hdkey->path_ = kMasterNode;
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
  const uint8_t* ptr = buf.data();
  auto version = static_cast<ExtendedKeyVersion>(ptr[0] << 24 | ptr[1] << 16 |
                                                 ptr[2] << 8 | ptr[3] << 0);
  ptr += sizeof(version);

  uint8_t depth = *ptr;
  ptr += sizeof(depth);

  int32_t parent_fingerprint =
      ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3] << 0;
  ptr += sizeof(parent_fingerprint);

  int32_t index = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3] << 0;
  ptr += sizeof(index);

  std::unique_ptr<HDKey> hdkey = std::make_unique<HDKey>();
  hdkey->depth_ = depth;
  hdkey->parent_fingerprint_ = parent_fingerprint;
  hdkey->index_ = index;

  std::vector<uint8_t> chain_code(ptr, ptr + 32);
  ptr += chain_code.size();
  hdkey->SetChainCode(chain_code);

  if (*ptr == 0x00) {
    // Skip first zero byte which is not part of private key.
    hdkey->SetPrivateKey(base::make_span(ptr + 1, ptr + 33));
  } else {
    hdkey->SetPublicKey(*base::span(ptr, ptr + kSecp256k1PubkeySize)
                             .to_fixed_extent<kSecp256k1PubkeySize>());
  }
  auto result = std::make_unique<ParsedExtendedKey>();
  result->hdkey = std::move(hdkey);
  result->version = version;
  return result;
}

// static
std::unique_ptr<HDKey> HDKey::GenerateFromPrivateKey(
    base::span<const uint8_t> private_key) {
  if (private_key.size() != 32) {
    return nullptr;
  }
  std::unique_ptr<HDKey> hd_key = std::make_unique<HDKey>();
  hd_key->SetPrivateKey(private_key);
  return hd_key;
}

// static
std::unique_ptr<HDKey> HDKey::GenerateFromV3UTC(const std::string& password,
                                                const std::string& json) {
  if (password.empty()) {
    VLOG(0) << __func__ << "empty password";
    return nullptr;
  }
  auto parsed_json = base::JSONReader::ReadAndReturnValueWithError(json);
  if (!parsed_json.has_value() || !parsed_json->is_dict()) {
    VLOG(0) << __func__ << ": UTC v3 json parsed failed because "
            << parsed_json.error().message;
    return nullptr;
  }

  auto& dict = parsed_json->GetDict();
  // check version
  auto version = dict.FindInt("version");
  if (!version || *version != 3) {
    VLOG(0) << __func__ << ": missing version or version is not 3";
    return nullptr;
  }

  const auto* crypto = dict.FindDict("crypto");
  if (!crypto) {
    VLOG(0) << __func__ << ": missing crypto";
    return nullptr;
  }
  const auto* kdf = crypto->FindString("kdf");
  if (!kdf) {
    VLOG(0) << __func__ << ": missing kdf";
    return nullptr;
  }
  const auto* kdfparams = crypto->FindDict("kdfparams");
  if (!kdfparams) {
    VLOG(0) << __func__ << ": missing kdfparams";
    return nullptr;
  }
  auto dklen = kdfparams->FindInt("dklen");
  if (!dklen) {
    VLOG(0) << __func__ << ": missing dklen";
    return nullptr;
  }
  if (*dklen != 32) {
    VLOG(0) << __func__ << ": dklen must be 32";
    return nullptr;
  }
  const auto* salt = kdfparams->FindString("salt");
  if (!salt) {
    VLOG(0) << __func__ << ": missing salt";
    return nullptr;
  }
  std::vector<uint8_t> salt_bytes;
  if (!base::HexStringToBytes(*salt, &salt_bytes)) {
    VLOG(1) << __func__ << ": invalid salt";
    return nullptr;
  }

  std::vector<uint8_t> key((size_t)*dklen);

  if (*kdf == "pbkdf2") {
    auto c = kdfparams->FindInt("c");
    if (!c) {
      VLOG(0) << __func__ << ": missing c";
      return nullptr;
    }
    const auto* prf = kdfparams->FindString("prf");
    if (!prf) {
      VLOG(0) << __func__ << ": missing prf";
      return nullptr;
    }
    if (*prf != "hmac-sha256") {
      VLOG(0) << __func__ << ": prf must be hmac-sha256 when using pbkdf2";
      return nullptr;
    }

    crypto::kdf::Pbkdf2HmacSha256Params params = {
        .iterations = base::checked_cast<decltype(params.iterations)>(*c),
    };
    if (!crypto::kdf::DeriveKeyPbkdf2HmacSha256(
            params, base::as_byte_span(password),
            base::as_byte_span(salt_bytes), key)) {
      VLOG(1) << __func__ << ": pbkdf2 derivation failed";
      return nullptr;
    }
  } else if (*kdf == "scrypt") {
    auto n = kdfparams->FindInt("n");
    if (!n) {
      VLOG(0) << __func__ << ": missing n";
      return nullptr;
    }
    auto r = kdfparams->FindInt("r");
    if (!r) {
      VLOG(0) << __func__ << ": missing r";
      return nullptr;
    }
    auto p = kdfparams->FindInt("p");
    if (!p) {
      VLOG(0) << __func__ << ": missing p";
      return nullptr;
    }
    crypto::kdf::ScryptParams params = {
        .cost = (size_t)*n,
        .block_size = (size_t)*r,
        .parallelization = (size_t)*p,
        .max_memory_bytes = 512 * 1024 * 1024,
    };
    if (!crypto::kdf::DeriveKeyScryptNoCheck(
            params, base::as_byte_span(password),
            base::as_byte_span(salt_bytes), key)) {
      VLOG(1) << __func__ << ": scrypt derivation failed";
      return nullptr;
    }
  } else {
    VLOG(0) << __func__
            << ": kdf is not supported. (Only support pbkdf2 and scrypt)";
    return nullptr;
  }

  const auto* mac = crypto->FindString("mac");
  if (!mac) {
    VLOG(0) << __func__ << ": missing mac";
    return nullptr;
  }
  const auto* ciphertext = crypto->FindString("ciphertext");
  if (!ciphertext) {
    VLOG(0) << __func__ << ": missing ciphertext";
    return nullptr;
  }
  std::vector<uint8_t> ciphertext_bytes;
  if (!base::HexStringToBytes(*ciphertext, &ciphertext_bytes)) {
    VLOG(1) << __func__ << ": invalid ciphertext";
    return nullptr;
  }

  auto derived_key = std::make_unique<SymmetricKey>(key);
  if (!UTCPasswordVerification(derived_key->key(), ciphertext_bytes, *mac,
                               *dklen)) {
    return nullptr;
  }

  const auto* cipher = crypto->FindString("cipher");
  if (!cipher) {
    VLOG(0) << __func__ << ": missing cipher";
    return nullptr;
  }
  if (*cipher != "aes-128-ctr") {
    VLOG(0) << __func__
            << ": AES-128-CTR is the minimal requirement of version 3";
    return nullptr;
  }

  std::vector<uint8_t> iv_bytes;
  const auto* iv = crypto->FindStringByDottedPath("cipherparams.iv");
  if (!iv) {
    VLOG(0) << __func__ << ": missing cipherparams.iv";
    return nullptr;
  }
  if (!base::HexStringToBytes(*iv, &iv_bytes)) {
    VLOG(1) << __func__ << ": invalid iv";
    return nullptr;
  }

  std::vector<uint8_t> private_key;
  if (!UTCDecryptPrivateKey(derived_key->key(), ciphertext_bytes, iv_bytes,
                            &private_key, *dklen)) {
    return nullptr;
  }

  return GenerateFromPrivateKey(private_key);
}

std::string HDKey::GetPath() const {
  return path_;
}

void HDKey::SetPrivateKey(base::span<const uint8_t> value) {
  if (value.size() != 32) {
    LOG(ERROR) << __func__ << ": pivate key must be 32 bytes";
    return;
  }
  private_key_.assign(value.begin(), value.end());
  GeneratePublicKey();
  identifier_ = base::ToVector(Hash160(public_key_));

  const uint8_t* ptr = identifier_.data();
  fingerprint_ = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3] << 0;
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
  identifier_ = base::ToVector(Hash160(public_key_));

  const uint8_t* ptr = identifier_.data();
  fingerprint_ = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3] << 0;
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

void HDKey::SetChainCode(base::span<const uint8_t> value) {
  chain_code_.assign(value.begin(), value.end());
}

std::unique_ptr<HDKey> HDKey::DeriveNormalChild(uint32_t index) {
  if (index >= kHardenedOffset) {
    return nullptr;
  }

  return DeriveChild(index);
}

std::unique_ptr<HDKey> HDKey::DeriveHardenedChild(uint32_t index) {
  if (index >= kHardenedOffset) {
    return nullptr;
  }

  return DeriveChild(kHardenedOffset + index);
}

std::unique_ptr<HDKey> HDKey::DeriveChild(uint32_t index) {
  bool is_hardened = index >= kHardenedOffset;
  SecureVector data;

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

  auto hmac = HmacSha512(chain_code_, data);
  auto [IL, IR] = base::span(hmac).split_at(kSHA512HashLength / 2);

  std::unique_ptr<HDKey> hdkey = std::make_unique<HDKey>();
  hdkey->SetChainCode(IR);

  if (!private_key_.empty()) {
    // Private parent key -> private child key
    // Also Private parent key -> public child key because we always create
    // public key.
    SecureVector private_key = private_key_;
    if (!secp256k1_ec_seckey_tweak_add(GetSecp256k1Ctx(), private_key.data(),
                                       IL.data())) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_seckey_tweak_add failed";
      return nullptr;
    }
    hdkey->SetPrivateKey(private_key);
  } else {
    // Public parent key -> public child key (Normal only)
    DCHECK(!is_hardened);
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(GetSecp256k1Ctx(), &pubkey,
                                   public_key_.data(), public_key_.size())) {
      LOG(ERROR) << __func__ << ": secp256k1_ec_pubkey_parse failed";
      return nullptr;
    }

    if (!secp256k1_ec_pubkey_tweak_add(GetSecp256k1Ctx(), &pubkey, IL.data())) {
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

  if (!path_.empty()) {
    const std::string node =
        is_hardened ? base::NumberToString(index - kHardenedOffset) + "'"
                    : base::NumberToString(index);

    hdkey->path_ = base::StrCat({path_, "/", node});
  }
  hdkey->depth_ = depth_ + 1;
  hdkey->parent_fingerprint_ = fingerprint_;
  hdkey->index_ = index;

  return hdkey;
}

std::unique_ptr<HDKey> HDKey::DeriveChildFromPath(const std::string& path) {
  if (path_ != kMasterNode) {
    LOG(ERROR) << __func__ << ": must derive only from master key";
    return nullptr;
  }
  if (private_key_.empty()) {
    LOG(ERROR) << __func__ << ": master key must have private key";
    return nullptr;
  }

  const auto hd_path = ParseFullHDPath(path);
  if (!hd_path) {
    return nullptr;
  }

  std::unique_ptr<HDKey> hd_key = std::make_unique<HDKey>();
  hd_key->SetPrivateKey(private_key_);
  hd_key->SetChainCode(chain_code_);
  hd_key->path_ = path_;

  for (auto node : *hd_path) {
    hd_key = hd_key->DeriveChild(node);
    if (!hd_key) {
      return nullptr;
    }
  }

  DCHECK_EQ(path, hd_key->GetPath());

  return hd_key;
}

std::vector<uint8_t> HDKey::SignCompact(base::span<const uint8_t> msg,
                                        int* recid) {
  std::vector<uint8_t> sig(kCompactSignatureSize);
  if (msg.size() != 32) {
    LOG(ERROR) << __func__ << ": message length should be 32";
    return sig;
  }
  if (!recid) {
    secp256k1_ecdsa_signature ecdsa_sig;
    if (!secp256k1_ecdsa_sign(GetSecp256k1Ctx(), &ecdsa_sig, msg.data(),
                              private_key_.data(),
                              secp256k1_nonce_function_rfc6979, nullptr)) {
      LOG(ERROR) << __func__ << ": secp256k1_ecdsa_sign failed";
      return sig;
    }

    if (!secp256k1_ecdsa_signature_serialize_compact(GetSecp256k1Ctx(),
                                                     sig.data(), &ecdsa_sig)) {
      LOG(ERROR) << __func__
                 << ": secp256k1_ecdsa_signature_serialize_compact failed";
    }
  } else {
    secp256k1_ecdsa_recoverable_signature ecdsa_sig;
    if (!secp256k1_ecdsa_sign_recoverable(
            GetSecp256k1Ctx(), &ecdsa_sig, msg.data(), private_key_.data(),
            secp256k1_nonce_function_rfc6979, nullptr)) {
      LOG(ERROR) << __func__ << ": secp256k1_ecdsa_sign_recoverable failed";
      return sig;
    }
    if (!secp256k1_ecdsa_recoverable_signature_serialize_compact(
            GetSecp256k1Ctx(), sig.data(), recid, &ecdsa_sig)) {
      LOG(ERROR)
          << __func__
          << ": secp256k1_ecdsa_recoverable_signature_serialize_compact failed";
    }
  }

  return sig;
}

std::optional<std::vector<uint8_t>> HDKey::SignDer(
    base::span<const uint8_t, 32> msg) {
  unsigned char extra_entropy[32] = {0};
  secp256k1_ecdsa_signature ecdsa_sig;
  if (!secp256k1_ecdsa_sign(GetSecp256k1Ctx(), &ecdsa_sig, msg.data(),
                            private_key_.data(),
                            secp256k1_nonce_function_rfc6979, nullptr)) {
    LOG(ERROR) << __func__ << ": secp256k1_ecdsa_sign failed";
    return std::nullopt;
  }

  auto sig_has_low_r = [](const secp256k1_context* ctx,
                          const secp256k1_ecdsa_signature* sig) {
    uint8_t compact_sig[kCompactSignatureSize] = {};
    secp256k1_ecdsa_signature_serialize_compact(ctx, compact_sig, sig);

    return compact_sig[0] < 0x80;
  };

  // Grind R https://github.com/bitcoin/bitcoin/pull/13666
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

bool HDKey::VerifyForTesting(base::span<const uint8_t> msg,
                             base::span<const uint8_t> sig) {
  if (msg.size() != 32 || sig.size() != kCompactSignatureSize) {
    LOG(ERROR) << __func__ << ": message or signature length is invalid";
    return false;
  }

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

std::vector<uint8_t> HDKey::RecoverCompact(bool compressed,
                                           base::span<const uint8_t> msg,
                                           base::span<const uint8_t> sig,
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
  SecureVector buf;

  buf.reserve(kSerializationLength);

  uint32_t version_uint32 = static_cast<uint32_t>(version);

  buf.push_back((version_uint32 >> 24) & 0xFF);
  buf.push_back((version_uint32 >> 16) & 0xFF);
  buf.push_back((version_uint32 >> 8) & 0xFF);
  buf.push_back((version_uint32 >> 0) & 0xFF);

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
  if (key.size() == 32) {
    DCHECK(version == ExtendedKeyVersion::kXprv ||
           version == ExtendedKeyVersion::kYprv ||
           version == ExtendedKeyVersion::kZprv);
    // 32-bytes private key is padded with a zero byte.
    buf.insert(buf.end(), 0);
  } else {
    DCHECK(version == ExtendedKeyVersion::kXpub ||
           version == ExtendedKeyVersion::kYpub ||
           version == ExtendedKeyVersion::kZpub);
    DCHECK_EQ(key.size(), 33u);
  }
  buf.insert(buf.end(), key.begin(), key.end());

  DCHECK(buf.size() == kSerializationLength);
  return EncodeBase58Check(buf);
}

}  // namespace brave_wallet

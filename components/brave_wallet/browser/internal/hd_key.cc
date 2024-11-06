/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key.h"

#include <array>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"
#include "brave/vendor/bat-native-tweetnacl/tweetnacl.h"
#include "crypto/encryptor.h"
#include "crypto/kdf.h"
#include "crypto/process_bound_string.h"
#include "crypto/random.h"
#include "crypto/symmetric_key.h"
#include "third_party/boringssl/src/include/openssl/hmac.h"

#define SECP256K1_BUILD  // This effectively turns off export attributes.
#include "brave/third_party/bitcoin-core/src/src/secp256k1/include/secp256k1.h"
#include "brave/third_party/bitcoin-core/src/src/secp256k1/include/secp256k1_recovery.h"

using crypto::Encryptor;
using crypto::SymmetricKey;

namespace base {
namespace internal {
template <size_t N>
struct ExtentImpl<brave_wallet::SecureByteArray<N>> : size_constant<N> {};
}  // namespace internal
}  // namespace base

namespace brave_wallet {

namespace {
constexpr char kMasterNode[] = "m";
constexpr char kMasterSecret[] = "Bitcoin seed";
constexpr size_t kSHA512Length = 64;
constexpr uint32_t kHardenedOffset = 0x80000000;
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

std::vector<uint8_t> GeneratePublicKey(Secp256k1PrivateKeySpan private_key) {
  secp256k1_pubkey public_key;
  if (!secp256k1_ec_pubkey_create(GetSecp256k1Ctx(), &public_key,
                                  private_key.data())) {
    LOG(ERROR) << "secp256k1_ec_pubkey_create failed";
    return {};
  }

  size_t public_key_len = kSecp256k1PubkeySize;
  std::vector<uint8_t> public_key_bytes(kSecp256k1PubkeySize);
  if (!secp256k1_ec_pubkey_serialize(GetSecp256k1Ctx(), public_key_bytes.data(),
                                     &public_key_len, &public_key,
                                     SECP256K1_EC_COMPRESSED)) {
    LOG(ERROR) << "secp256k1_ec_pubkey_serialize failed";
  }

  return public_key_bytes;
}

bool UTCPasswordVerification(const std::string& derived_key,
                             base::span<const uint8_t> ciphertext,
                             base::span<const uint8_t, kKeccakHashLength> mac,
                             size_t dklen) {
  std::vector<uint8_t> mac_verification_input(derived_key.end() - dklen / 2,
                                              derived_key.end());
  mac_verification_input.insert(mac_verification_input.end(),
                                ciphertext.begin(), ciphertext.end());
  // verify password
  if (KeccakHash(mac_verification_input) != mac) {
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

HDKey::HDKey() = default;
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

  SecureByteArray<kSHA512Length> hmac;
  unsigned int out_len;
  if (!HMAC(EVP_sha512(), kMasterSecret, sizeof(kMasterSecret), seed.data(),
            seed.size(), hmac.data(), &out_len)) {
    LOG(ERROR) << __func__ << ": HMAC_SHA512 failed";
    return nullptr;
  }
  DCHECK(out_len == kSHA512Length);

  std::unique_ptr<HDKey> hdkey = std::make_unique<HDKey>();
  auto [IL, IR] = base::span(hmac).split_at<kSHA512Length / 2>();
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

  auto reader = base::SpanReader(base::as_byte_span(buf));

  // version(4) || depth(1) || parent_fingerprint(4) || index(4) || chain(32) ||
  // key(33)

  auto result = std::make_unique<ParsedExtendedKey>();
  result->hdkey = std::make_unique<HDKey>();

  uint32_t version = 0;
  if (!reader.ReadU32BigEndian(version)) {
    return nullptr;
  }
  result->version = static_cast<ExtendedKeyVersion>(version);

  if (!reader.ReadU8BigEndian(result->hdkey->depth_)) {
    return nullptr;
  }

  if (!reader.ReadCopy(result->hdkey->parent_fingerprint_)) {
    return nullptr;
  }

  if (!reader.ReadU32BigEndian(result->hdkey->index_)) {
    return nullptr;
  }

  auto chain_code = reader.Read<kBip32ChainCodeSize>();
  if (!chain_code) {
    return nullptr;
  }
  result->hdkey->SetChainCode(*chain_code);

  auto key_bytes = reader.Read<kSecp256k1PubkeySize>();
  if (!key_bytes) {
    return nullptr;
  }
  if (key_bytes->front() == 0x00) {
    // Skip first zero byte which is not part of private key.
    result->hdkey->SetPrivateKey(key_bytes->subspan<1>());
  } else {
    result->hdkey->SetPublicKey(*key_bytes);
  }

  return result;
}

// static
std::unique_ptr<HDKey> HDKey::GenerateFromPrivateKey(
    Secp256k1PrivateKeySpan private_key) {
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

  const auto* mac_hex = crypto->FindString("mac");
  if (!mac_hex) {
    VLOG(0) << __func__ << ": missing mac";
    return nullptr;
  }
  std::array<uint8_t, kKeccakHashLength> mac;
  if (!base::HexStringToSpan(*mac_hex, mac)) {
    VLOG(0) << __func__ << ": invalid mac";
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
  if (!UTCPasswordVerification(derived_key->key(), ciphertext_bytes, mac,
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

  auto private_key_span = base::as_byte_span(private_key)
                              .to_fixed_extent<kSecp256k1PrivateKeySize>();
  if (!private_key_span) {
    return nullptr;
  }

  return GenerateFromPrivateKey(*private_key_span);
}

std::string HDKey::GetPath() const {
  return path_;
}

void HDKey::SetPrivateKey(Secp256k1PrivateKeySpan value) {
  private_key_.emplace();
  base::span(*private_key_).copy_from(value);
  public_key_ = GeneratePublicKey(*private_key_);
}

std::string HDKey::GetPrivateExtendedKey(ExtendedKeyVersion version) const {
  if (!private_key_) {
    return {};
  }
  return Serialize(version, *private_key_);
}

std::vector<uint8_t> HDKey::GetPrivateKeyBytes() const {
  if (!private_key_) {
    return {};
  }
  return std::vector<uint8_t>(private_key_->begin(), private_key_->end());
}

std::vector<uint8_t> HDKey::GetPublicKeyBytes() const {
  DCHECK(!public_key_.empty());
  return public_key_;
}

void HDKey::SetPublicKey(Secp256k1PubkeyKeySpan value) {
  // Verify public key
  secp256k1_pubkey pubkey;
  if (!secp256k1_ec_pubkey_parse(GetSecp256k1Ctx(), &pubkey, value.data(),
                                 value.size())) {
    LOG(ERROR) << __func__ << ": not a valid public key";
    return;
  }
  public_key_.assign(value.begin(), value.end());
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
  if (!private_key_ || crypto_scalarmult_curve25519_tweet_base(
                           public_key.data(), private_key_->data()) != 0) {
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
  if (!private_key_) {
    return std::nullopt;
  }
  static_assert(kSecp256k1PrivateKeySize ==
                crypto_box_curve25519xsalsa20poly1305_tweet_SECRETKEYBYTES);

  std::vector<uint8_t> padded_ciphertext(ciphertext.begin(), ciphertext.end());
  padded_ciphertext.insert(padded_ciphertext.begin(), crypto_box_BOXZEROBYTES,
                           0);
  std::vector<uint8_t> padded_plaintext(padded_ciphertext.size());
  if (crypto_box_open(padded_plaintext.data(), padded_ciphertext.data(),
                      padded_ciphertext.size(), nonce.data(),
                      ephemeral_public_key.data(), private_key_->data()) != 0) {
    return std::nullopt;
  }
  std::vector<uint8_t> plaintext(
      padded_plaintext.cbegin() + crypto_box_ZEROBYTES,
      padded_plaintext.cend());
  return plaintext;
}

void HDKey::SetChainCode(Bip32ChainCodeSpan value) {
  base::span(chain_code_).copy_from(value);
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
    CHECK(private_key_);
    data.push_back(0x00);
    data.insert(data.end(), private_key_->begin(), private_key_->end());
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

  SecureByteArray<kSHA512Length> hmac;
  unsigned int out_len;
  if (!HMAC(EVP_sha512(), chain_code_.data(), chain_code_.size(), data.data(),
            data.size(), hmac.data(), &out_len)) {
    LOG(ERROR) << __func__ << ": HMAC_SHA512 failed";
    return nullptr;
  }
  DCHECK(out_len == kSHA512Length);

  auto [IL, IR] = base::span(hmac).split_at<kSHA512Length / 2>();

  std::unique_ptr<HDKey> hdkey = std::make_unique<HDKey>();
  hdkey->SetChainCode(IR);

  if (private_key_) {
    // Private parent key -> private child key
    // Also Private parent key -> public child key because we always create
    // public key.
    SecureByteArray<kSecp256k1PrivateKeySize> private_key = *private_key_;
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
  hdkey->parent_fingerprint_ = GetFingerprint();
  hdkey->index_ = index;

  return hdkey;
}

std::unique_ptr<HDKey> HDKey::DeriveChildFromPath(const std::string& path) {
  if (path_ != kMasterNode) {
    LOG(ERROR) << __func__ << ": must derive only from master key";
    return nullptr;
  }
  if (!private_key_) {
    LOG(ERROR) << __func__ << ": master key must have private key";
    return nullptr;
  }

  std::unique_ptr<HDKey> hd_key = std::make_unique<HDKey>();
  std::vector<std::string> entries =
      base::SplitString(path, "/", base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);
  if (entries.empty()) {
    return nullptr;
  }

  // Starting with 'm' node and effectively copying `*this` into `hd_key`.
  if (entries[0] != kMasterNode) {
    LOG(ERROR) << __func__ << ": path must start with \"m\"";
    return nullptr;
  }

  hd_key->SetPrivateKey(*private_key_);
  hd_key->SetChainCode(chain_code_);
  hd_key->path_ = path_;

  for (size_t i = 1; i < entries.size(); ++i) {
    std::string entry = entries[i];

    bool is_hardened = entry.length() > 1 && entry.back() == '\'';
    if (is_hardened) {
      entry.pop_back();
    }
    unsigned child_index = 0;
    if (!base::StringToUint(entry, &child_index)) {
      LOG(ERROR) << __func__ << ": path must contain number or number'";
      return nullptr;
    }
    if (child_index >= kHardenedOffset) {
      LOG(ERROR) << __func__ << ": index must be less than " << kHardenedOffset;
      return nullptr;
    }
    if (is_hardened) {
      child_index += kHardenedOffset;
    }

    hd_key = hd_key->DeriveChild(child_index);
    if (!hd_key) {
      return nullptr;
    }
  }

  DCHECK_EQ(path, hd_key->GetPath());

  return hd_key;
}

std::optional<std::array<uint8_t, kCompactSignatureSize>> HDKey::SignCompact(
    Secp256k1SignMsgSpan msg,
    int* recid) {
  CHECK(private_key_);

  std::array<uint8_t, kCompactSignatureSize> sig = {};
  if (!recid) {
    secp256k1_ecdsa_signature ecdsa_sig;
    if (!secp256k1_ecdsa_sign(GetSecp256k1Ctx(), &ecdsa_sig, msg.data(),
                              private_key_->data(),
                              secp256k1_nonce_function_rfc6979, nullptr)) {
      LOG(ERROR) << __func__ << ": secp256k1_ecdsa_sign failed";
      return std::nullopt;
    }

    if (!secp256k1_ecdsa_signature_serialize_compact(GetSecp256k1Ctx(),
                                                     sig.data(), &ecdsa_sig)) {
      LOG(ERROR) << __func__
                 << ": secp256k1_ecdsa_signature_serialize_compact failed";
    }
  } else {
    secp256k1_ecdsa_recoverable_signature ecdsa_sig;
    if (!secp256k1_ecdsa_sign_recoverable(
            GetSecp256k1Ctx(), &ecdsa_sig, msg.data(), private_key_->data(),
            secp256k1_nonce_function_rfc6979, nullptr)) {
      LOG(ERROR) << __func__ << ": secp256k1_ecdsa_sign_recoverable failed";
      return std::nullopt;
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

std::optional<std::vector<uint8_t>> HDKey::SignDer(Secp256k1SignMsgSpan msg) {
  CHECK(private_key_);

  unsigned char extra_entropy[32] = {0};
  secp256k1_ecdsa_signature ecdsa_sig;
  if (!secp256k1_ecdsa_sign(GetSecp256k1Ctx(), &ecdsa_sig, msg.data(),
                            private_key_->data(),
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
            GetSecp256k1Ctx(), &ecdsa_sig, msg.data(), private_key_->data(),
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

std::vector<uint8_t> HDKey::RecoverCompact(bool compressed,
                                           Secp256k1SignMsgSpan msg,
                                           CompactSignatureSpan sig,
                                           int recid) {
  size_t public_key_len = compressed ? 33 : 65;
  std::vector<uint8_t> public_key(public_key_len);

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

// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#key-identifiers
std::array<uint8_t, kBip32IdentifierSize> HDKey::GetIdentifier() {
  return Hash160(public_key_);
}

// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#key-identifiers
std::array<uint8_t, kBip32FingerprintSize> HDKey::GetFingerprint() {
  std::array<uint8_t, kBip32FingerprintSize> fingerprint;
  CHECK(base::SpanReader(base::as_byte_span(GetIdentifier()))
            .ReadCopy(fingerprint));
  return fingerprint;
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

  buf.insert(buf.end(), parent_fingerprint_.begin(), parent_fingerprint_.end());

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

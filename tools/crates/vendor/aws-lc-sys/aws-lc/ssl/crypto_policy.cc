// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#include <openssl/ssl.h>

#include "internal.h"

// This file implements opt-in seeding of new |SSL_CTX| objects from the system
// crypto-policies OpenSSL back-end (Amazon Linux 2023 and Fedora). It is only
// compiled with -DENABLE_CRYPTO_POLICIES; otherwise it is an (almost) empty
// translation unit. |internal.h| is included unconditionally so the unit always
// carries declarations and never trips empty-translation-unit diagnostics.

#if defined(AWSLC_CRYPTO_POLICIES)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(OPENSSL_WINDOWS)
#include <unistd.h>
#endif

#include <openssl/err.h>

#include "../crypto/err/internal.h"

#if defined(OPENSSL_LINUX)
#include "../crypto/fipsmodule/cpucap/cpu_getauxval_linux.h"
#endif

BSSL_NAMESPACE_BEGIN

namespace {

// kMaxCachedPathLen bounds the policy path the cache can key on. A longer path
// is parsed uncached rather than rejected.
constexpr size_t kMaxCachedPathLen = 1023;

// PolicyCache holds the last policy file read whole, keyed on the path it came
// from.
struct PolicyCache {
  char path[kMaxCachedPathLen + 1];
  CryptoPolicyConfig cfg;
  bool valid;  // the fields above are filled in
};

struct CRYPTO_STATIC_MUTEX g_policy_cache_lock = CRYPTO_STATIC_MUTEX_INIT;
PolicyCache g_policy_cache;  // Guarded by |g_policy_cache_lock|.

// LoadPolicy fills |out| with the policy at |path|, parsing the file only on a
// cache miss, and returns whether |path| named a readable file.
//
// A policy read whole is read once per process for a given path. OpenSSL
// likewise parses openssl.cnf at library init rather than per |SSL_CTX|, and
// update-crypto-policies already requires restarting consumers for a new policy
// to take effect, so nothing observes the difference. Keying on the path keeps
// the AWSLC_CRYPTO_POLICY_FILE override live: a changed path misses.
//
// Only a successful parse is cached. A transient read error would otherwise
// decide the policy for every |SSL_CTX| the process goes on to create, and no
// errno is reliable enough to tell that case from a file that is simply absent.
// The price is a failing open() per |SSL_CTX_new| on a host that has no policy.
bool LoadPolicy(const char *path, CryptoPolicyConfig *out) {
  const size_t path_len = strlen(path);
  if (path_len > kMaxCachedPathLen) {
    return ssl_crypto_policy_parse_file(path, out);
  }

  CRYPTO_STATIC_MUTEX_lock_read(&g_policy_cache_lock);
  const bool hit =
      g_policy_cache.valid && strcmp(g_policy_cache.path, path) == 0;
  if (hit) {
    OPENSSL_memcpy(out, &g_policy_cache.cfg, sizeof(*out));
  }
  CRYPTO_STATIC_MUTEX_unlock_read(&g_policy_cache_lock);
  if (hit) {
    return true;
  }

  CryptoPolicyConfig cfg = {};
  if (!ssl_crypto_policy_parse_file(path, &cfg)) {
    return false;
  }

  // A concurrent miss on a different path may have populated the cache in the
  // meantime; overwriting it is harmless, since either entry is correct for the
  // path it names.
  CRYPTO_STATIC_MUTEX_lock_write(&g_policy_cache_lock);
  OPENSSL_memcpy(g_policy_cache.path, path, path_len + 1);
  OPENSSL_memcpy(&g_policy_cache.cfg, &cfg, sizeof(cfg));
  g_policy_cache.valid = true;
  CRYPTO_STATIC_MUTEX_unlock_write(&g_policy_cache_lock);

  OPENSSL_memcpy(out, &cfg, sizeof(cfg));
  return true;
}

// IsAsciiWhitespace matches the horizontal and line-ending whitespace that can
// appear around a directive in an OpenSSL config file.
bool IsAsciiWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// CopyPolicyValue copies the |len| bytes at |value| into |out|, which has
// capacity |out_size| including the NUL terminator. It returns true on success.
//
// An overlong value empties |out| rather than truncating it. The last occurrence
// of a directive is the one the operator chose, so leaving an earlier occurrence
// in place would apply a policy they had already replaced.
bool CopyPolicyValue(char *out, size_t out_size, const char *value,
                     size_t len) {
  if (len >= out_size) {
    out[0] = '\0';
    return false;
  }
  OPENSSL_memcpy(out, value, len);
  out[len] = '\0';
  return true;
}

// CryptoPolicyProtoVersion maps a crypto-policies protocol token (e.g.
// "TLSv1.2", "DTLSv1.2") to the corresponding AWS-LC version constant, or 0 if
// the token is unrecognized or unsupported (e.g. "DTLSv1.3", for which AWS-LC
// has no constant).
uint16_t CryptoPolicyProtoVersion(const char *tok, bool is_dtls) {
  if (is_dtls) {
    if (strcmp(tok, "DTLSv1") == 0 || strcmp(tok, "DTLSv1.0") == 0) {
      return DTLS1_VERSION;
    }
    if (strcmp(tok, "DTLSv1.2") == 0) {
      return DTLS1_2_VERSION;
    }
    return 0;
  }
  if (strcmp(tok, "TLSv1") == 0 || strcmp(tok, "TLSv1.0") == 0) {
    return TLS1_VERSION;
  }
  if (strcmp(tok, "TLSv1.1") == 0) {
    return TLS1_1_VERSION;
  }
  if (strcmp(tok, "TLSv1.2") == 0) {
    return TLS1_2_VERSION;
  }
  if (strcmp(tok, "TLSv1.3") == 0) {
    return TLS1_3_VERSION;
  }
  return 0;
}

// kMaxPolicyIds bounds the algorithm IDs kept from one policy directive. No
// directive can yield more than the number of groups or signature algorithms
// AWS-LC implements, which is well under this.
constexpr size_t kMaxPolicyIds = 64;

// kPolicyListSeparators end a token in a Groups or SignatureAlgorithms value.
// ':' separates entries, and '/' separates the tuples OpenSSL 3.5 draws its key
// shares from. AWS-LC keeps one flat preference list and chooses its own key
// shares, so a tuple boundary is just another entry boundary; treating it as
// part of a name instead loses the entries on both sides of it.
constexpr char kPolicyListSeparators[] = ":/";

// PolicyAlias maps one spelling the crypto-policies framework uses to the
// spelling AWS-LC's lookups take. |from| is lowercase and matched without regard
// to case, which config-file tokens do not carry reliably.
struct PolicyAlias {
  const char *from;
  const char *to;
};

// crypto-policies uses the IANA registry name for the NIST P-256 curve, which
// AWS-LC knows only as "P-256" and "prime256v1", and hyphenates the ML-KEM
// hybrids. Without these the most widely deployed group in the list, and every
// post-quantum group, resolve to nothing.
const PolicyAlias kGroupAliases[] = {
    {"secp256r1", "P-256"},
    {"x25519-mlkem768", "X25519MLKEM768"},
    {"secp256r1-mlkem768", "SecP256r1MLKEM768"},
    {"secp384r1-mlkem1024", "SecP384r1MLKEM1024"},
};

// crypto-policies spells ML-DSA with the FIPS 204 parameter-set names.
const PolicyAlias kSigalgAliases[] = {
    {"ml-dsa-44", "mldsa44"},
    {"ml-dsa-65", "mldsa65"},
    {"ml-dsa-87", "mldsa87"},
};

// kPolicyMLDSASigalgs are AWS-LC's default ML-DSA signature algorithms.
const uint16_t kPolicyMLDSASigalgs[] = {SSL_SIGN_MLDSA44, SSL_SIGN_MLDSA65,
                                        SSL_SIGN_MLDSA87};

bool EqualsIgnoreAsciiCase(const char *tok, size_t len, const char *lower) {
  if (strlen(lower) != len) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    char c = tok[i];
    if (c >= 'A' && c <= 'Z') {
      c = static_cast<char>(c + ('a' - 'A'));
    }
    if (c != lower[i]) {
      return false;
    }
  }
  return true;
}

// ResolveAlias rewrites |*tok| and |*len| through |aliases| if one matches.
void ResolveAlias(const char **tok, size_t *len, const PolicyAlias *aliases,
                  size_t num_aliases) {
  for (size_t i = 0; i < num_aliases; i++) {
    if (EqualsIgnoreAsciiCase(*tok, *len, aliases[i].from)) {
      *tok = aliases[i].to;
      *len = strlen(aliases[i].to);
      return;
    }
  }
}

// StripListModifiers advances |*tok| past the OpenSSL 3.5 list modifiers and
// returns false if they remove the entry.
//
// '*' asks for a key share and '?' tolerates an unimplemented algorithm; AWS-LC
// chooses its own key shares and already skips names it cannot resolve, so both
// need only stripping. A policy stacks them -- Amazon Linux 2023's PQ subpolicy
// leads its group list with "*?X25519MLKEM768" -- so one pass per prefix
// character is not enough. '-' removes the entry, and resolving it here would
// put it back in the list.
bool StripListModifiers(const char **tok, size_t *len) {
  while (*len > 0 && (**tok == '*' || **tok == '?')) {
    (*tok)++;
    (*len)--;
  }
  return *len == 0 || **tok != '-';
}

// GroupIdFromToken sets |*out| to the AWS-LC group ID named by the
// crypto-policies token |tok|, of length |len|, and returns false if AWS-LC has
// no such group.
bool GroupIdFromToken(uint16_t *out, const char *tok, size_t len) {
  if (!StripListModifiers(&tok, &len)) {
    return false;
  }

  ResolveAlias(&tok, &len, kGroupAliases, OPENSSL_ARRAY_SIZE(kGroupAliases));
  return ssl_name_to_group_id(out, tok, len);
}

// SigalgIdFromToken sets |*out| to the AWS-LC signature algorithm ID named by
// the crypto-policies token |tok|, of length |len|, and returns false if AWS-LC
// has no such algorithm.
bool SigalgIdFromToken(uint16_t *out, const char *tok, size_t len) {
  if (!StripListModifiers(&tok, &len)) {
    return false;
  }

  ResolveAlias(&tok, &len, kSigalgAliases, OPENSSL_ARRAY_SIZE(kSigalgAliases));
  return ssl_sigalg_id_from_name(out, tok, len);
}

bool ContainsId(Span<const uint16_t> ids, uint16_t id) {
  for (uint16_t candidate : ids) {
    if (candidate == id) {
      return true;
    }
  }
  return false;
}

// FilterPolicyIds resolves the tokens of |value| through |lookup| and writes the
// IDs that resolve into |out|, which holds |out_len| entries, in the order the
// policy gave them. It returns how many were written.
//
// The Groups and SignatureAlgorithms setters reject a whole list on the first
// entry they do not accept, and a stock crypto-policies value always names
// something AWS-LC does not implement: X448 and the FFDHE groups, Ed448, the
// SHA-224 pairs, and the RSA-PSS-PSS algorithms. Applying such a value as written
// therefore discards the operator's whole preference order. Dropping the
// unsupported tokens keeps the rest of it.
//
// Repeats are dropped for the same reason: two spellings of one group, such as
// "secp256r1" and "prime256v1", resolve to a single ID, and |SSL_CTX_set1_group_ids|
// rejects a list that names it twice.
size_t FilterPolicyIds(uint16_t *out, size_t out_len, const char *value,
                       bool (*lookup)(uint16_t *, const char *, size_t)) {
  size_t out_i = 0;
  for (const char *tok = value;;) {
    const size_t len = strcspn(tok, kPolicyListSeparators);

    uint16_t id;
    if (len > 0 && out_i < out_len && lookup(&id, tok, len)) {
      bool seen = false;
      for (size_t i = 0; i < out_i; i++) {
        seen = seen || out[i] == id;
      }
      if (!seen) {
        out[out_i++] = id;
      }
    }

    if (tok[len] == '\0') {
      break;
    }
    tok += len + 1;
  }
  return out_i;
}

// PolicyKeepsPQDefaults reports whether |cfg| leaves AWS-LC's post-quantum
// defaults in force. Only AWS-LC's own AWSLC.PostQuantum directive waives them
// wholesale: a crypto-policies list can remove a group it names, but nothing in
// the directives the framework writes says "no post-quantum".
bool PolicyKeepsPQDefaults(const CryptoPolicyConfig &cfg) {
  return !EqualsIgnoreAsciiCase(cfg.post_quantum, strlen(cfg.post_quantum),
                                "off");
}

// HybridClassicalComponent sets |*out| to the non-post-quantum half of the
// hybrid group |group| and returns false if |group| is not a hybrid.
bool HybridClassicalComponent(uint16_t *out, uint16_t group) {
  for (const HybridGroup &hybrid : HybridGroups()) {
    if (hybrid.group_id != group) {
      continue;
    }
    for (uint16_t component : hybrid.component_group_ids) {
      if (!ContainsId(PQGroups(), component)) {
        *out = component;
        return true;
      }
    }
  }
  return false;
}

// ValueNamesRemoval reports whether |value| names |id| with the OpenSSL '-'
// modifier, resolving its tokens through |lookup|.
bool ValueNamesRemoval(const char *value, uint16_t id,
                       bool (*lookup)(uint16_t *, const char *, size_t)) {
  for (const char *tok = value;;) {
    const size_t len = strcspn(tok, kPolicyListSeparators);
    uint16_t candidate;
    if (len > 1 && tok[0] == '-' && lookup(&candidate, tok + 1, len - 1) &&
        candidate == id) {
      return true;
    }
    if (tok[len] == '\0') {
      return false;
    }
    tok += len + 1;
  }
}

// PolicyRemovesGroup reports whether the Groups value |value| takes |group| out.
//
// A hybrid goes out with its classical half. Removing X25519 forbids X25519 key
// exchange, and X25519MLKEM768 still performs it on the wire, so leaving the
// hybrid in place would make the removal cosmetic.
bool PolicyRemovesGroup(const char *value, uint16_t group) {
  uint16_t classical;
  return ValueNamesRemoval(value, group, GroupIdFromToken) ||
         (HybridClassicalComponent(&classical, group) &&
          ValueNamesRemoval(value, classical, GroupIdFromToken));
}

// DropRemovedGroups compacts |ids|, which holds |n| entries, down to the ones the
// Groups value |value| does not remove, and returns how many are left.
size_t DropRemovedGroups(uint16_t *ids, size_t n, const char *value) {
  size_t kept = 0;
  for (size_t i = 0; i < n; i++) {
    if (!PolicyRemovesGroup(value, ids[i])) {
      ids[kept++] = ids[i];
    }
  }
  return kept;
}

// DropRemovedSigalgs compacts |ids|, which holds |n| entries, down to the ones
// the SignatureAlgorithms value |value| does not remove, and returns how many
// are left.
size_t DropRemovedSigalgs(uint16_t *ids, size_t n, const char *value) {
  size_t kept = 0;
  for (size_t i = 0; i < n; i++) {
    if (!ValueNamesRemoval(value, ids[i], SigalgIdFromToken)) {
      ids[kept++] = ids[i];
    }
  }
  return kept;
}

// PolicySigalgIds fills |out|, which holds |out_len| entries, with the
// signature algorithms the SignatureAlgorithms value |value| leaves in force,
// starting from |defaults| when the value names none of its own, and returns
// how many were written, or zero to leave AWS-LC's defaults implicit.
//
// This is |PolicyGroupIds| for signature algorithms, and for the same reason: a
// value that only removes has to be applied to the default list, or the
// algorithm the operator took out comes back.
size_t PolicySigalgIds(uint16_t *out, size_t out_len, const char *value,
                       Span<const uint16_t> defaults) {
  size_t n = FilterPolicyIds(out, out_len, value, SigalgIdFromToken);
  const bool named_any = n != 0;
  if (!named_any) {
    for (uint16_t sigalg : defaults) {
      if (n >= out_len) {
        return 0;
      }
      out[n++] = sigalg;
    }
  }

  const size_t kept = DropRemovedSigalgs(out, n, value);
  if (kept == 0 || (!named_any && kept == n)) {
    return 0;
  }
  return kept;
}

// PolicyGroupIds fills |out|, which holds |out_len| entries, with the groups the
// Groups value |value| leaves in force and returns how many were written, or zero
// to leave AWS-LC's defaults implicit.
//
// The groups |value| names are its preference order, and its '-' entries come out
// of that order. A value that only removes is applied to the default list, since
// skipping the directive would hand back the group the operator took out.
//
// A value that removes every group also gets zero: an empty configured list is how
// AWS-LC spells "use the defaults", so a policy leaving no group at all is one this
// layer cannot express and does not apply.
size_t PolicyGroupIds(uint16_t *out, size_t out_len, const char *value) {
  size_t n = FilterPolicyIds(out, out_len, value, GroupIdFromToken);
  const bool named_any = n != 0;
  if (!named_any) {
    for (uint16_t group : tls1_get_default_grouplist()) {
      if (n >= out_len) {
        return 0;
      }
      out[n++] = group;
    }
  }

  const size_t kept = DropRemovedGroups(out, n, value);
  // A value that removed nothing from the defaults asked for nothing this layer
  // can act on, so the defaults stay implicit rather than frozen into |ctx|.
  if (kept == 0 || (!named_any && kept == n)) {
    return 0;
  }
  return kept;
}

// MergeDefaultPQGroups restores AWS-LC's default post-quantum groups at the front
// of |ids|, which holds |n| of |cap| entries and came from the Groups value
// |value|, and returns the new count.
//
// The group setter replaces AWS-LC's defaults rather than intersecting with them,
// so without this a policy written before ML-KEM existed silently downgrades
// every context. A policy naming any post-quantum group has an opinion about
// them and is left alone, as does one that names a group only to remove it.
//
// A hybrid whose classical half the policy dropped is not restored: an operator
// who removed P-384 did not ask for P-384 key exchange back under another name.
size_t MergeDefaultPQGroups(uint16_t *ids, size_t n, size_t cap,
                            const char *value) {
  if (n == 0) {
    return n;
  }
  for (size_t i = 0; i < n; i++) {
    if (ContainsId(PQGroups(), ids[i])) {
      return n;
    }
  }

  uint16_t add[kMaxPolicyIds];
  size_t num_add = 0;
  for (uint16_t group : tls1_get_default_grouplist()) {
    if (!ContainsId(PQGroups(), group)) {
      continue;
    }
    uint16_t classical;
    if (HybridClassicalComponent(&classical, group) &&
        !ContainsId(MakeConstSpan(ids, n), classical)) {
      continue;
    }
    if (PolicyRemovesGroup(value, group)) {
      continue;
    }
    if (num_add < OPENSSL_ARRAY_SIZE(add)) {
      add[num_add++] = group;
    }
  }
  if (num_add == 0 || n + num_add > cap) {
    return n;
  }

  OPENSSL_memmove(ids + num_add, ids, n * sizeof(uint16_t));
  OPENSSL_memcpy(ids, add, num_add * sizeof(uint16_t));
  return n + num_add;
}

// MergeDefaultPQSigalgs restores AWS-LC's default ML-DSA algorithms at the end of
// |ids|, which holds |n| of |cap| entries and came from the SignatureAlgorithms
// value |value|, and returns the new count. As with groups, a policy naming any of
// them is left alone, and one it names only to remove stays out.
//
// They go last because that is where AWS-LC's own default list puts them.
size_t MergeDefaultPQSigalgs(uint16_t *ids, size_t n, size_t cap,
                             const char *value) {
  if (n == 0) {
    return n;
  }
  for (uint16_t sigalg : kPolicyMLDSASigalgs) {
    if (ContainsId(MakeConstSpan(ids, n), sigalg)) {
      return n;
    }
  }

  uint16_t add[OPENSSL_ARRAY_SIZE(kPolicyMLDSASigalgs)];
  size_t num_add = 0;
  for (uint16_t sigalg : kPolicyMLDSASigalgs) {
    if (!ValueNamesRemoval(value, sigalg, SigalgIdFromToken)) {
      add[num_add++] = sigalg;
    }
  }
  if (num_add == 0 || n + num_add > cap) {
    return n;
  }

  OPENSSL_memcpy(ids + n, add, num_add * sizeof(uint16_t));
  return n + num_add;
}

// ApplyCipherRule applies the cipher rule |rule| to |ctx|, as
// |SSL_CTX_set_cipher_list| does when |config_tls13| is false and
// |SSL_CTX_set_ciphersuites| when it is true, and returns false having left |ctx|
// as it was if any step fails.
//
// Neither public setter is a no-op on failure. |ssl_create_cipher_list| installs
// its result, empty or not, before reporting that the rule matched nothing, and
// the |update_cipher_list| that merges the TLS 1.2 and TLS 1.3 lists back
// together allocates, so it can fail after the first list is already in place.
// Either way the context is left holding part of a policy it could not apply,
// which for the first is no ciphers at all. Building both lists aside and moving
// them in once every step has succeeded is what keeps a failure to the defaults.
bool ApplyCipherRule(SSL_CTX *ctx, const char *rule, bool config_tls13) {
  const bool has_aes_hw = ctx->aes_hw_override ? ctx->aes_hw_override_value
                                               : EVP_has_aes_hardware();
  UniquePtr<SSLCipherPreferenceList> configured;
  if (!ssl_create_cipher_list(&configured, has_aes_hw, rule,
                              false /* not strict */, config_tls13)) {
    return false;
  }

  UniquePtr<SSLCipherPreferenceList> &tls12_list =
      config_tls13 ? ctx->cipher_list : configured;
  UniquePtr<SSLCipherPreferenceList> &tls13_list =
      config_tls13 ? configured : ctx->tls13_cipher_list;
  UniquePtr<SSLCipherPreferenceList> merged;
  if (!update_cipher_list(merged, tls12_list, tls13_list)) {
    return false;
  }

  if (config_tls13) {
    ctx->tls13_cipher_list = std::move(configured);
  }
  ctx->cipher_list = std::move(merged);
  return true;
}

// PolicyNamesOlderProtocol matches the names the crypto-policies OpenSSL
// back-end renders for protocols older than any AWS-LC implements. The context's
// own floor already sits above these, so such a floor needs no raising.
bool PolicyNamesOlderProtocol(const char *tok) {
  return strcmp(tok, "SSLv2") == 0 || strcmp(tok, "SSLv3") == 0 ||
         strcmp(tok, "DTLSv0.9") == 0;
}

// ApplyPolicyVersionBounds seeds the protocol version floor and ceiling from
// |cfg|, choosing the TLS.* or DTLS.* directives per |is_dtls|.
void ApplyPolicyVersionBounds(SSL_CTX *ctx, const CryptoPolicyConfig &cfg,
                              bool is_dtls, bool version_locked) {
  // A version-locked SSL_METHOD (TLSv1_2_method and friends) is skipped
  // entirely. |SSL_CTX_new| pins such a context by setting both bounds to
  // |method->version|, but the public setters validate only against the protocol
  // method's full version range, so applying a policy here would quietly raise
  // the ceiling and hand the caller a version they deliberately excluded.
  if (version_locked) {
    return;
  }

  const char *min_tok = is_dtls ? cfg.dtls_min : cfg.tls_min;
  uint16_t policy_min = CryptoPolicyProtoVersion(min_tok, is_dtls);
  const uint16_t policy_max =
      CryptoPolicyProtoVersion(is_dtls ? cfg.dtls_max : cfg.tls_max, is_dtls);

  // The ceiling the pair is resolved against, and the floor a policy AWS-LC
  // cannot read gets.
  const uint16_t effective_max =
      policy_max != 0 ? policy_max : ctx->conf_max_version;

  // A floor the operator wrote and AWS-LC cannot resolve rises to the ceiling,
  // the strictest floor that still leaves the context able to handshake. Leaving
  // it alone fails open: the built-in floor is TLS 1.0 (DTLS 1.0), below every
  // floor the framework renders, so a name AWS-LC has no version for --
  // "DTLSv1.3" today, any newer protocol later -- would hand back the versions
  // the policy exists to forbid.
  //
  // An unresolvable ceiling stays unapplied, since lowering a ceiling AWS-LC
  // cannot read would take away the strongest protocol on offer.
  if (policy_min == 0 &&
      (is_dtls ? cfg.dtls_min_present : cfg.tls_min_present) &&
      !PolicyNamesOlderProtocol(min_tok)) {
    policy_min = effective_max;
  }

  // The setters check each bound against the method's whole version range and
  // never against each other, so a policy whose floor sits above its ceiling
  // would be accepted and leave every later handshake failing with
  // SSL_R_NO_SUPPORTED_VERSIONS_ENABLED. Resolve the pair first, filling in
  // whichever end the policy omits from the bound the context already has, and
  // apply it only if the resulting range is non-empty.
  //
  // The comparison is on protocol versions rather than wire versions because
  // DTLS wire values run backwards -- DTLS 1.0 is 0xfeff and DTLS 1.2 is 0xfefd
  // -- so ordering the raw values would invert the test for DTLS.
  uint16_t min_proto, max_proto;
  if (!ssl_protocol_version_from_wire(
          &min_proto, policy_min != 0 ? policy_min : ctx->conf_min_version) ||
      !ssl_protocol_version_from_wire(&max_proto, effective_max) ||
      min_proto > max_proto) {
    return;
  }

  // Each setter still refuses a version its method does not support, and the two
  // are independent, so a pair only one of them accepts would leave the context
  // with half a policy. Put the bounds back if either refuses.
  const uint16_t saved_min = ctx->conf_min_version;
  const uint16_t saved_max = ctx->conf_max_version;
  const bool saved_min_default = ctx->conf_min_version_use_default;
  const bool saved_max_default = ctx->conf_max_version_use_default;
  if ((policy_min != 0 && !SSL_CTX_set_min_proto_version(ctx, policy_min)) ||
      (policy_max != 0 && !SSL_CTX_set_max_proto_version(ctx, policy_max))) {
    ctx->conf_min_version = saved_min;
    ctx->conf_max_version = saved_max;
    ctx->conf_min_version_use_default = saved_min_default;
    ctx->conf_max_version_use_default = saved_max_default;
  }
}

// ApplyPolicyToCtx seeds |ctx| from the policy file at |path|. Seeding is
// best-effort: every failure is swallowed and the built-in default kept.
// |ssl_ctx_apply_crypto_policy| wraps this and cleans up the error queue.
void ApplyPolicyToCtx(SSL_CTX *ctx, const char *path, bool is_dtls,
                      bool version_locked) {
  CryptoPolicyConfig cfg = {};
  if (!LoadPolicy(path, &cfg)) {
    // Missing or unreadable policy file: keep the built-in defaults.
    return;
  }

  // CipherString. crypto-policies emits a leading "@SECLEVEL=N" token; AWS-LC
  // has no security levels and its cipher-list parser rejects '@' rules other
  // than "@STRENGTH", so the token must be stripped before the remainder is
  // applied. The non-strict setter is used deliberately so cipher aliases
  // AWS-LC does not recognize (e.g. "kEECDH", "-aDSS") are skipped rather than
  // fatal.
  if (cfg.cipher_string[0] != '\0') {
    const char *cs = cfg.cipher_string;
    if (strncmp(cs, "@SECLEVEL=", 10) == 0) {
      const char *colon = strchr(cs, ':');
      cs = colon != nullptr ? colon + 1 : "";
    }
    if (*cs != '\0') {
      ApplyCipherRule(ctx, cs, /*config_tls13=*/false);
    }
  }

  // Ciphersuites (TLS 1.3).
  if (cfg.ciphersuites[0] != '\0') {
    ApplyCipherRule(ctx, cfg.ciphersuites, /*config_tls13=*/true);
  }

  ApplyPolicyVersionBounds(ctx, cfg, is_dtls, version_locked);

  // SignatureAlgorithms and Groups, each narrowed to the algorithms AWS-LC
  // implements.
  const bool keep_pq = PolicyKeepsPQDefaults(cfg);
  if (cfg.sigalgs[0] != '\0') {
    // Both preference lists get the policy, matching what
    // |SSL_CTX_set1_sigalgs_list| writes. Each resolves on its own: AWS-LC
    // signs with a different default list than it accepts, so a value that only
    // removes reaches a different set of algorithms in each.
    uint16_t sign_ids[kMaxPolicyIds], verify_ids[kMaxPolicyIds];
    size_t sign_n =
        PolicySigalgIds(sign_ids, OPENSSL_ARRAY_SIZE(sign_ids), cfg.sigalgs,
                        tls12_get_default_sign_sigalgs());
    size_t verify_n =
        PolicySigalgIds(verify_ids, OPENSSL_ARRAY_SIZE(verify_ids), cfg.sigalgs,
                        tls12_get_default_verify_sigalgs());
    if (keep_pq) {
      sign_n = MergeDefaultPQSigalgs(sign_ids, sign_n,
                                     OPENSSL_ARRAY_SIZE(sign_ids), cfg.sigalgs);
      verify_n = MergeDefaultPQSigalgs(
          verify_ids, verify_n, OPENSSL_ARRAY_SIZE(verify_ids), cfg.sigalgs);
    }
    // Each list is a separate allocation, so the verify setter can fail with
    // the signing list already in place. Moving the signing list aside costs
    // nothing and is what lets that failure keep the defaults.
    Array<uint16_t> saved_signing;
    bool ok = true;
    if (sign_n > 0) {
      saved_signing = std::move(ctx->cert->sigalgs);
      ok = SSL_CTX_set_signing_algorithm_prefs(ctx, sign_ids, sign_n);
    }
    if (ok && verify_n > 0) {
      ok = SSL_CTX_set_verify_algorithm_prefs(ctx, verify_ids, verify_n);
    }
    if (!ok && sign_n > 0) {
      ctx->cert->sigalgs = std::move(saved_signing);
    }
  }
  if (cfg.groups[0] != '\0') {
    uint16_t ids[kMaxPolicyIds];
    size_t n = PolicyGroupIds(ids, OPENSSL_ARRAY_SIZE(ids), cfg.groups);
    if (keep_pq) {
      n = MergeDefaultPQGroups(ids, n, OPENSSL_ARRAY_SIZE(ids), cfg.groups);
    }
    if (n > 0) {
      SSL_CTX_set1_group_ids(ctx, ids, n);
    }
  }
}

}  // namespace

size_t ssl_crypto_policy_named_group_ids(uint16_t *out, size_t max_out,
                                         const char *value) {
  const size_t n = FilterPolicyIds(out, max_out, value, GroupIdFromToken);
  return DropRemovedGroups(out, n, value);
}

size_t ssl_crypto_policy_named_sigalg_ids(uint16_t *out, size_t max_out,
                                          const char *value) {
  const size_t n = FilterPolicyIds(out, max_out, value, SigalgIdFromToken);
  return DropRemovedSigalgs(out, n, value);
}

bool ssl_crypto_policy_parse_file(const char *path, CryptoPolicyConfig *out) {
  if (path == nullptr || out == nullptr) {
    return false;
  }

  FILE *f = fopen(path, "r");
  if (f == nullptr) {
    return false;
  }

  char buf[8192];
  while (fgets(buf, sizeof(buf), f) != nullptr) {
    size_t len = strlen(buf);
    bool had_newline = len > 0 && buf[len - 1] == '\n';
    // Skip over-long lines: if the buffer filled without reaching a newline and
    // we are not at end-of-file, consume the rest of the line and ignore it.
    if (!had_newline && feof(f) == 0) {
      int c;
      while ((c = fgetc(f)) != EOF && c != '\n') {
      }
      continue;
    }

    // Trim leading whitespace so indented directives are recognized, then skip
    // blank lines, comments, and section headers.
    const char *line = buf;
    while (*line == ' ' || *line == '\t') {
      line++;
    }
    if (*line == '\0' || *line == '\n' || *line == '#' || *line == '[') {
      continue;
    }

    const char *eq = strchr(line, '=');
    if (eq == nullptr) {
      continue;
    }

    // Trim whitespace off both sides of the key.
    const char *key = line;
    const char *key_end = eq;
    while (key < key_end && IsAsciiWhitespace(*key)) {
      key++;
    }
    while (key_end > key && IsAsciiWhitespace(key_end[-1])) {
      key_end--;
    }

    // Trim whitespace off both sides of the value, then strip a single pair of
    // surrounding quotes if present.
    const char *val = eq + 1;
    const char *val_end = line + strlen(line);
    while (val < val_end && IsAsciiWhitespace(*val)) {
      val++;
    }
    while (val_end > val && IsAsciiWhitespace(val_end[-1])) {
      val_end--;
    }
    if (val_end - val >= 2 && (*val == '"' || *val == '\'') &&
        *val == val_end[-1]) {
      val++;
      val_end--;
    }

    const size_t key_len = static_cast<size_t>(key_end - key);
    const size_t val_len = static_cast<size_t>(val_end - val);

    // KeyIs compares the trimmed key against a literal directive name.
    auto key_is = [key, key_len](const char *name) {
      return strlen(name) == key_len && OPENSSL_memcmp(key, name, key_len) == 0;
    };

    // Recognized directives; the last occurrence of a key wins. Unknown keys,
    // and values too long to represent, are ignored.
    if (key_is("CipherString")) {
      CopyPolicyValue(out->cipher_string, sizeof(out->cipher_string), val,
                      val_len);
    } else if (key_is("Ciphersuites")) {
      CopyPolicyValue(out->ciphersuites, sizeof(out->ciphersuites), val,
                      val_len);
    } else if (key_is("TLS.MinProtocol")) {
      out->tls_min_present = true;
      CopyPolicyValue(out->tls_min, sizeof(out->tls_min), val, val_len);
    } else if (key_is("TLS.MaxProtocol")) {
      CopyPolicyValue(out->tls_max, sizeof(out->tls_max), val, val_len);
    } else if (key_is("DTLS.MinProtocol")) {
      out->dtls_min_present = true;
      CopyPolicyValue(out->dtls_min, sizeof(out->dtls_min), val, val_len);
    } else if (key_is("DTLS.MaxProtocol")) {
      CopyPolicyValue(out->dtls_max, sizeof(out->dtls_max), val, val_len);
    } else if (key_is("SignatureAlgorithms")) {
      CopyPolicyValue(out->sigalgs, sizeof(out->sigalgs), val, val_len);
    } else if (key_is("Groups")) {
      CopyPolicyValue(out->groups, sizeof(out->groups), val, val_len);
    } else if (key_is("AWSLC.PostQuantum")) {
      CopyPolicyValue(out->post_quantum, sizeof(out->post_quantum), val,
                      val_len);
    }
  }

  // |fgets| and |fgetc| stop on a read error exactly as they stop at end of
  // file, so without this a truncated read would pass for a whole policy.
  const bool ok = ferror(f) == 0;
  fclose(f);
  return ok;
}

const char *ssl_crypto_policy_default_path(void) {
  // The compile-time default is a root-owned file under /etc; the environment is
  // not. Honoring the override in a process that gained privileges on exec
  // would let an unprivileged caller choose the TLS policy that privileged
  // process runs under, so the override is dropped across a privilege boundary.
#if defined(OPENSSL_HAS_GETAUXVAL)
  // The kernel sets AT_SECURE for every secure execution and never clears it,
  // so this also covers what leaves the ids below equal: file capabilities, and
  // a set-uid program that has already dropped privileges.
  if (getauxval(AT_SECURE) != 0) {
    return AWSLC_CRYPTO_POLICY_DEFAULT_FILE;
  }
#endif
#if !defined(OPENSSL_WINDOWS)
  if (getuid() != geteuid() || getgid() != getegid()) {
    return AWSLC_CRYPTO_POLICY_DEFAULT_FILE;
  }
#endif
  const char *env = getenv("AWSLC_CRYPTO_POLICY_FILE");
  if (env != nullptr && env[0] != '\0') {
    return env;
  }
  return AWSLC_CRYPTO_POLICY_DEFAULT_FILE;
}

void ssl_ctx_apply_crypto_policy(SSL_CTX *ctx, const char *path, bool is_dtls,
                                 bool version_locked) {
  if (ctx == nullptr || path == nullptr) {
    return;
  }

  // Seeding is best-effort, so the errors its failures queue must not reach the
  // caller. Neither may the caller's own queue be disturbed, since this runs
  // inside |SSL_CTX_new|, which no caller expects to touch the error queue at
  // all. Suppressing rather than trimming afterward is what keeps that true for
  // a caller whose queue is already full: there, each error seeding raised would
  // evict one of theirs, and no trim can bring an evicted entry back.
  //
  // Seeding is what gives way when the scope cannot be opened, since running it
  // unprotected is the one outcome the caller must not see.
  ScopedErrorSuppression suppress;
  if (!suppress) {
    return;
  }

  ApplyPolicyToCtx(ctx, path, is_dtls, version_locked);
}

BSSL_NAMESPACE_END

#endif  // AWSLC_CRYPTO_POLICIES

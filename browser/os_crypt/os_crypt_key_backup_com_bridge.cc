/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/os_crypt_key_backup_com_bridge.h"

#include <windows.h>

#include <wincrypt.h>

#include <algorithm>
#include <utility>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/win/scoped_localalloc.h"
#include "chrome/browser/os_crypt/app_bound_encryption_provider_win.h"
#include "chrome/browser/os_crypt/app_bound_encryption_win.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"

namespace brave {

namespace {

// Must match `kDPAPIKeyPrefix` in
// components/os_crypt/async/browser/dpapi_key_provider.cc and
// `kEncryptionVersionPrefix`/header handling in
// components/os_crypt/async/browser/os_crypt_win.cc.
constexpr uint8_t kDPAPIKeyPrefix[] = {'D', 'P', 'A', 'P', 'I'};

// Matches `DecryptKeyWithDPAPI` in
// components/os_crypt/async/browser/dpapi_key_provider.cc: a small, stable
// wrapper around one Win32 API that is unlikely to change shape across
// Chromium version bumps. Duplicating it avoids a patch that would need to
// remove that function's anonymous namespace and add a declaration to a
// header, which is a larger, ongoing-maintenance patch than duplicating ~15
// lines of code (see brave/docs/best-practices/chromium-src-overrides.md
// CSRC-001, CSRC-004, CSRC-023).
std::optional<std::vector<uint8_t>> DecryptWithDPAPI(
    base::span<const uint8_t> ciphertext) {
  DATA_BLOB input = {};
  input.pbData = const_cast<BYTE*>(ciphertext.data());
  input.cbData = static_cast<DWORD>(ciphertext.size());

  DATA_BLOB output;
  if (!::CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0,
                            &output)) {
    return std::nullopt;
  }

  auto local_alloc = base::win::TakeLocalAlloc(output.pbData);
  // SAFETY: `::CryptUnprotectData` is a raw Win32 API with no span-aware or
  // owning-buffer overload available - it hands back a `BYTE*`/length pair
  // with no safer alternative to construct a span from. Reaching this point
  // means `CryptUnprotectData` returned success, so `output.cbData` is the
  // exact byte length it reported for the `output.pbData` allocation it just
  // produced, and the pointer and length are guaranteed to describe the same
  // valid allocation.
  auto decrypted =
      UNSAFE_BUFFERS(base::span(local_alloc.get(), size_t{output.cbData}));
  return std::vector<uint8_t>(decrypted.begin(), decrypted.end());
}

// Matches `GetCurrentProtectionLevel` in
// chrome/browser/os_crypt/app_bound_encryption_provider_win.cc, which is
// private to that file. Duplicated for the same reason as `DecryptWithDPAPI`:
// a small, stable helper with no public API of its own.
ProtectionLevel GetCurrentProtectionLevel() {
  return base::FeatureList::IsEnabled(
             os_crypt_async::features::kEncryptWithIsolatedState)
             ? PROTECTION_PATH_VALIDATION_WITH_ISOLATION
             : PROTECTION_PATH_VALIDATION;
}

// How long to wait for the elevation service to respond before giving up on
// this history entry. Bounded so an unresponsive elevation service cannot
// hang browser startup indefinitely - the caller treats a timeout the same
// as a failed decrypt and moves on to the next-older history entry (or
// concludes none verify).
constexpr base::TimeDelta kAppBoundUnwrapTimeout = base::Seconds(10);

// Holds the result of a `DecryptAppBoundString` call made on a COM-STA
// background thread, and signals `completion_event` once it's ready. Ref
// counted because the posted task and the waiting caller each need a
// reference that can outlive the other (the caller may stop waiting on
// timeout while the task is still running).
struct DecryptAppBoundResult
    : public base::RefCountedThreadSafe<DecryptAppBoundResult> {
  base::WaitableEvent completion_event;
  std::optional<std::vector<uint8_t>> unwrapped_key;

 private:
  friend class base::RefCountedThreadSafe<DecryptAppBoundResult>;
  ~DecryptAppBoundResult() = default;
};

void DecryptAppBoundKeyOnCOMSTA(std::string ciphertext,
                                scoped_refptr<DecryptAppBoundResult> result) {
  const absl::Cleanup signal_completion = [&result] {
    result->completion_event.Signal();
  };

  std::string plaintext;
  DWORD last_error = 0;
  std::optional<std::string> new_ciphertext;
  elevation_service::EncryptFlags flags;
  const HRESULT hr = os_crypt::DecryptAppBoundString(
      ciphertext, plaintext, GetCurrentProtectionLevel(), new_ciphertext,
      last_error, &flags);
  if (!SUCCEEDED(hr)) {
    LOG(WARNING) << "App-bound key backup unwrap failed. Result: "
                 << logging::SystemErrorCodeToString(hr)
                 << " GetLastError: " << last_error;
    return;
  }

  result->unwrapped_key.emplace(plaintext.begin(), plaintext.end());
}

}  // namespace

std::optional<std::vector<uint8_t>> UnwrapWithDPAPI(
    base::span<const uint8_t> wrapped_key) {
  if (wrapped_key.size() < std::size(kDPAPIKeyPrefix) ||
      !std::ranges::equal(wrapped_key.first(std::size(kDPAPIKeyPrefix)),
                          base::span(kDPAPIKeyPrefix))) {
    return std::nullopt;
  }
  return DecryptWithDPAPI(wrapped_key.subspan(std::size(kDPAPIKeyPrefix)));
}

std::optional<std::vector<uint8_t>> BlockingUnwrapAppBoundKeyOnCOMSTA(
    base::span<const uint8_t> wrapped_key,
    std::string_view step_description) {
  if (wrapped_key.size() < sizeof(os_crypt_async::kCryptAppBoundKeyPrefix) ||
      !std::ranges::equal(
          wrapped_key.first(sizeof(os_crypt_async::kCryptAppBoundKeyPrefix)),
          base::span(os_crypt_async::kCryptAppBoundKeyPrefix))) {
    return std::nullopt;
  }
  base::span<const uint8_t> ciphertext_bytes =
      wrapped_key.subspan(sizeof(os_crypt_async::kCryptAppBoundKeyPrefix));
  std::string ciphertext(ciphertext_bytes.begin(), ciphertext_bytes.end());

  VLOG(1) << "OSCrypt key backup: " << step_description
          << " (blocking on elevation service)";

  auto result = base::MakeRefCounted<DecryptAppBoundResult>();
  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTask(FROM_HERE, base::BindOnce(&DecryptAppBoundKeyOnCOMSTA,
                                           std::move(ciphertext), result));

  const bool completed =
      result->completion_event.TimedWait(kAppBoundUnwrapTimeout);
  VLOG(1) << "OSCrypt key backup: " << step_description
          << (completed ? " finished" : " timed out");
  if (!completed) {
    return std::nullopt;
  }

  return result->unwrapped_key;
}

}  // namespace brave

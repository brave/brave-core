/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_IDENTITY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_IDENTITY_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/auto_reset.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/process/process_handle.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "base/process/process.h"
#elif BUILDFLAG(IS_MAC)
#include <mach/message.h>
#elif BUILDFLAG(IS_LINUX)
#include "base/files/scoped_file.h"
#endif

namespace named_mojo_ipc_server {
struct ConnectionInfo;
}  // namespace named_mojo_ipc_server

namespace brave_vpn::v2 {

// BrowserIdentity is a reference to the process on the far end of one browser
// connection, taken while that connection is being accepted. All member
// functions are cheap and non-blocking; the expensive part of verification is
// posted to a thread pool and reported via a callback.
//
// Refcounted as its ownership is genuinely shared: one accept-time capture is
// reachable from BrowserRegistry's pid-keyed capture map and from every
// connection that resolved against it, and those connections outlive the
// capture.
class BrowserIdentity : public base::RefCounted<BrowserIdentity> {
 public:
  enum class VerificationResult {
    // Verified as Brave browser.
    kAccepted,
    // Verified as NOT a Brave browser: a verdict about the binary, which does
    // not change while it runs.
    kRejected,
    // Could not tell: image replaced mid-update, cert rotated, etc. A later
    // attempt may succeed, so this must never be treated as a rejection.
    kInconclusive,
  };

  using VerificationRequestCallback = base::OnceCallback<VerificationResult()>;
  using VerificationResponseCallback =
      base::OnceCallback<void(VerificationResult)>;

  // Creates a BrowserIdentity for the peer described by |info| by invoking the
  // capture function. Returns null if the capture fails. The capture function
  // can be overriden in tests by SetBrowserIdentityCaptureCallbackForTesting().
  static scoped_refptr<BrowserIdentity> Create(
      const named_mojo_ipc_server::ConnectionInfo& info);

  BrowserIdentity(const BrowserIdentity&) = delete;
  BrowserIdentity& operator=(const BrowserIdentity&) = delete;

  // The peer's pid. Only meaningful as a lookup key: a pid is not an identity,
  // and one read at dispatch time may already name a different process than the
  // one that connected.
  base::ProcessId pid() const { return pid_; }

  // Informational string for logging: a pid, a timestamp, etc.; no sensitive
  // information.
  virtual std::string GetDescription() const;

  // True if |other| names the same process instance, not merely the same pid.
  // Used to notice that a cached identity went stale because its pid was
  // recycled by an unrelated process.
  // Fail-closed: returns false if either identity carries no platform data.
  virtual bool IsSameProcess(const BrowserIdentity& other) const;

  // Posts an expensive part of verification to a thread pool with all the
  // necessary platform-specific arguments, and calls |callback| with the
  // result. |callback| is never invoked synchronously, and always runs on the
  // sequence that called Verify(), which must therefore have a current default
  // task runner. Safe to call more than once, and on any number of identities
  // concurrently. Only meaningful on a capture taken at accept time: a capture
  // taken while a message is dispatching may carry no platform data beyond the
  // pid, since the ConnectionInfo the server keeps as receiver context has had
  // its endpoint consumed by the invitation.
  virtual void Verify(VerificationResponseCallback callback) const;

 protected:
  friend class base::RefCounted<BrowserIdentity>;

  // PlatformData is what pins one process instance, beyond its pid.
  struct PlatformData {
#if BUILDFLAG(IS_WIN)
    // Held open so the pid cannot be reused while this identity is alive.
    base::Process process;
    // Raw FILETIME creation time (100 ns intervals since 1601-01-01 UTC).
    uint64_t creation_time = 0;
#elif BUILDFLAG(IS_MAC)
    // Carries pid and pidversion; also the race-free handle for code-signing
    // checks during verification.
    audit_token_t audit_token = {};
#elif BUILDFLAG(IS_LINUX)
    // Refers to the original process even if its pid number is later reused.
    base::ScopedFD pidfd;
    // /proc/<pid>/stat field 22: clock ticks since boot. Fixed for the life of
    // the process and independent of the wall clock.
    uint64_t start_time_ticks = 0;
#else
#error "BrowserIdentity is not supported on this platform"
#endif
  };

  // Captures a BrowserIdentity for the peer described by |info|, with
  // platform-specific data, or returns null if the peer cannot be pinned or is
  // not running as this user.
  static scoped_refptr<BrowserIdentity> Capture(
      const named_mojo_ipc_server::ConnectionInfo& info);

  explicit BrowserIdentity(base::ProcessId pid);
  BrowserIdentity(base::ProcessId pid, PlatformData platform_data);
  virtual ~BrowserIdentity();

  // Returns a closure carrying copies of the platform data the blocking
  // verification needs, so it can run without touching the object.
  VerificationRequestCallback BindVerificationRequest() const;

  const base::ProcessId pid_;
  const std::optional<PlatformData> platform_data_;
};

using BrowserIdentityCaptureCallback =
    base::RepeatingCallback<scoped_refptr<BrowserIdentity>(
        const named_mojo_ipc_server::ConnectionInfo& info)>;

[[nodiscard]] base::AutoReset<BrowserIdentityCaptureCallback>
SetBrowserIdentityCaptureCallbackForTesting(  // IN-TEST
    BrowserIdentityCaptureCallback callback);

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_IDENTITY_H_

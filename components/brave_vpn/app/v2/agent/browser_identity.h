/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_IDENTITY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_IDENTITY_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/process/process_handle.h"

namespace named_mojo_ipc_server {
struct ConnectionInfo;
}  // namespace named_mojo_ipc_server

namespace brave_vpn::v2 {

// BrowserIdentity is a reference to the process on the far end of one browser
// connection, taken while that connection is being accepted. Abstract because
// an identity is platform-specific.
//
// Refcounted because an instance is captured on the IPC sequence, read on a
// blocking pool, and outlives the registry if the agent shuts down while a
// verification is in flight.
class BrowserIdentity : public base::RefCountedThreadSafe<BrowserIdentity> {
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

  BrowserIdentity(const BrowserIdentity&) = delete;
  BrowserIdentity& operator=(const BrowserIdentity&) = delete;

  // The peer's pid. Only meaningful as a lookup key: a pid is not an identity,
  // and one read at dispatch time may already name a different process than the
  // one that connected.
  virtual base::ProcessId pid() const = 0;

  // Informational string for logging: a pid, a timestamp, etc.; no sensitive
  // information.
  virtual std::string GetDescription() const = 0;

  // Runs the expensive half of verification: the code identity of the process
  // this object names; blocking.
  virtual VerificationResult Verify() const = 0;

  // True if |other| names the same process instance, not merely the same pid.
  // Used to notice that a cached identity went stale because its pid was
  // recycled by an unrelated process.
  virtual bool IsSameProcess(const BrowserIdentity& other) const = 0;

 protected:
  friend class base::RefCountedThreadSafe<BrowserIdentity>;

  BrowserIdentity() = default;
  virtual ~BrowserIdentity() = default;
};

// BrowserIdentityFactory is the only entity that knows what a ConnectionInfo
// holds. ConnectionInfo belongs to named_mojo_ipc_server and carries the peer's
// credentials in a different member on every platform, so keeping the knowledge
// behind one seam is what lets BrowserRegistry stay platform-neutral and lets
// unit tests describe a peer without fabricating kernel data.
class BrowserIdentityFactory {
 public:
  BrowserIdentityFactory(const BrowserIdentityFactory&) = delete;
  BrowserIdentityFactory& operator=(const BrowserIdentityFactory&) = delete;

  virtual ~BrowserIdentityFactory() = default;

  // Captures the peer described by |info|, or returns null if the peer cannot
  // be pinned or is not running as this user; cheap and non-blocking.
  //
  // Called from two places, and |info| is not equally complete in both: the
  // accept callback has the live endpoint, while the copy the server keeps as
  // receiver context has had it consumed by the invitation. Which part of the
  // connection info is valid is a Mojo IPC implementation detail, so only pid()
  // and the comparison in IsSameProcess() are guaranteed on a capture taken at
  // dispatch. Hence verification calls must only be performed on accept-time
  // captures.
  virtual scoped_refptr<BrowserIdentity> Capture(
      const named_mojo_ipc_server::ConnectionInfo& info) const = 0;

 protected:
  BrowserIdentityFactory() = default;
};

// Defined once per platform.
std::unique_ptr<BrowserIdentityFactory> CreateBrowserIdentityFactory();

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_IDENTITY_H_

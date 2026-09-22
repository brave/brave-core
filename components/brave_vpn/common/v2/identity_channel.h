/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_V2_IDENTITY_CHANNEL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_V2_IDENTITY_CHANNEL_H_

#include <optional>

#include "build/build_config.h"
#include "mojo/public/cpp/platform/platform_handle.h"

#if BUILDFLAG(IS_MAC)
#include <mach/message.h>
#endif  // BUILDFLAG(IS_MAC)

namespace brave_vpn::v2 {

#if BUILDFLAG(IS_MAC)

// The two ends of one identity channel; both are Mach rights to the same port.
struct IdentityChannel {
  mojo::PlatformHandle receive_handle;
  mojo::PlatformHandle send_handle;
};

// Creates a fresh identity channel for one server identity message, or nullopt
// if the port could not be allocated. The send right is handed over on the IPC
// pipe itself, so only that pipe's peer can receive it. A send right can be
// forwarded on, so "the message came from the peer of this pipe" rests on two
// things:
// - The server answers the channel only after it has verified the client, so a
//   process that cannot pass that check cannot have the real server stamp a
//   token onto a port it collected from someone else.
// - Only one process can hold the server's endpoint name at a time, so there is
//   no second server to relay to.
std::optional<IdentityChannel> CreateIdentityChannel();

// Reads the server's identity message from |receive_handle| and returns the
// audit token the kernel stamped on it, or nullopt if no message is waiting or
// it is not the message this contract describes. Never blocks: by the time the
// server's reply reaches the client the message is already queued, so an empty
// port means no identity was supplied and the caller must fail closed.
std::optional<audit_token_t> ReadIdentityMessage(
    mojo::PlatformHandle receive_handle);

// Sends the IPC server's identity message on |send_handle|, if a client asked
// to be told who is serving it by handing one over. The message body is empty
// on purpose: the client reads the Mac audit token appended by the kernel to
// the message trailer, which cannot be forged by the sender. Does not block.
void SendIdentityMessage(mojo::PlatformHandle send_handle);

#endif  // BUILDFLAG(IS_MAC)

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_V2_IDENTITY_CHANNEL_H_

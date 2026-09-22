/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/v2/identity_channel.h"

#include <bsm/libbsm.h>
#include <mach/mach.h>
#include <unistd.h>

#include <utility>

#include "base/apple/scoped_mach_port.h"
#include "base/test/test_timeouts.h"
#include "mojo/public/cpp/platform/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {

// A client that is not asking to verify the server sends a null handle, which
// the server must simply ignore. A handle holding no Mach send right would
// CHECK if taken, so the point is that the call returns.
TEST(IdentityChannelMacTest, SendIgnoresNullChannel) {
  SendIdentityMessage(mojo::PlatformHandle());
}

// The audit token is the only thing a server identity message carries, and it
// is the client's sole evidence of who served it. Both ends are this process
// here, so this covers only the Mach plumbing.
TEST(IdentityChannelMacTest, MessageTrailerCarriesSenderAuditToken) {
  base::apple::ScopedMachSendRight send_right;
  base::apple::ScopedMachReceiveRight receive_right;
  {
    const kern_return_t kr = mach_port_allocate(
        mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
        base::apple::ScopedMachReceiveRight::Receiver(receive_right).get());
    ASSERT_EQ(KERN_SUCCESS, kr);
  }
  {
    const kern_return_t kr =
        mach_port_insert_right(mach_task_self(), receive_right.get(),
                               receive_right.get(), MACH_MSG_TYPE_MAKE_SEND);
    ASSERT_EQ(KERN_SUCCESS, kr);
    send_right = base::apple::ScopedMachSendRight(receive_right.get());
  }

  SendIdentityMessage(mojo::PlatformHandle(std::move(send_right)));

  // The kernel appends the trailer immediately after the message, and this
  // message is header-only, so the trailer lands exactly on |message.trailer|.
  // Sized to the largest trailer format so any trailer the kernel offers fits.
  struct {
    mach_msg_header_t header;
    mach_msg_max_trailer_t trailer;
  } message{};
  static_assert(
      round_msg(sizeof(mach_msg_header_t)) == sizeof(mach_msg_header_t),
      "a header-only message needs no trailer offset arithmetic");

  ASSERT_EQ(KERN_SUCCESS,
            mach_msg(&message.header,
                     MACH_RCV_MSG | MACH_RCV_TIMEOUT |
                         MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0) |
                         MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AUDIT),
                     /*send_size=*/0, sizeof(message), receive_right.get(),
                     static_cast<mach_msg_timeout_t>(
                         TestTimeouts::action_timeout().InMilliseconds()),
                     MACH_PORT_NULL));

  // Both guard the assumption above: a body would move the trailer, and a
  // short trailer would mean the audit token was not filled in.
  ASSERT_EQ(sizeof(mach_msg_header_t), message.header.msgh_size);
  ASSERT_GE(message.trailer.msgh_trailer_size,
            sizeof(mach_msg_audit_trailer_t));

  EXPECT_EQ(getpid(), audit_token_to_pid(message.trailer.msgh_audit));
}

}  // namespace brave_vpn::v2

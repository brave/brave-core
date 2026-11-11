/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/v2/identity_channel_mac.h"

#include <bsm/libbsm.h>
#include <mach/mach.h>
#include <unistd.h>

#include <optional>
#include <utility>

#include "base/apple/scoped_mach_port.h"
#include "base/test/test_timeouts.h"
#include "mojo/public/cpp/platform/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {

// A fresh port with no messages queued, for the cases that send by hand. The
// caller keeps the receive right, so the port stays alive as long as the test
// needs it.
base::apple::ScopedMachReceiveRight AllocateReceiveRight() {
  base::apple::ScopedMachReceiveRight receive_right;
  const kern_return_t kr = mach_port_allocate(
      mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
      base::apple::ScopedMachReceiveRight::Receiver(receive_right).get());
  EXPECT_EQ(KERN_SUCCESS, kr);
  return receive_right;
}

// A send right to |port|, whose receive right the caller keeps. Inserting
// MAKE_SEND puts the send right under the same name, with one user reference:
// the caller owns that reference and releases it when the returned right goes
// out of scope.
base::apple::ScopedMachSendRight MakeSendRight(mach_port_t port) {
  const kern_return_t kr = mach_port_insert_right(mach_task_self(), port, port,
                                                  MACH_MSG_TYPE_MAKE_SEND);
  EXPECT_EQ(KERN_SUCCESS, kr);
  return base::apple::ScopedMachSendRight(kr == KERN_SUCCESS ? port
                                                             : MACH_PORT_NULL);
}

// Sends |message| to the port |send_right| names, for the malformed shapes
// SendIdentityMessage() cannot produce. Fills in the fields every case shares.
void SendRawMessage(mach_port_t send_right, mach_msg_header_t* message) {
  message->msgh_bits |= MACH_MSGH_BITS_REMOTE(MACH_MSG_TYPE_COPY_SEND);
  message->msgh_remote_port = send_right;
  message->msgh_id = 0;

  const kern_return_t kr =
      mach_msg(message, MACH_SEND_MSG | MACH_SEND_TIMEOUT, message->msgh_size,
               /*rcv_size=*/0, MACH_PORT_NULL, /*timeout=*/0, MACH_PORT_NULL);
  EXPECT_EQ(KERN_SUCCESS, kr);
}

}  // namespace

// A client that is not asking to verify the server sends a null handle, which
// the server must simply ignore. The guard is what makes that safe; the point
// is that the call returns.
TEST(IdentityChannelMacTest, SendIgnoresNullChannel) {
  SendIdentityMessage(mojo::PlatformHandle());
}

// The audit token is the only thing a server identity message carries, and it
// is the client's sole evidence of who served it. Both ends are this process
// here, so this covers only the Mach plumbing.
TEST(IdentityChannelMacTest, MessageTrailerCarriesSenderAuditToken) {
  base::apple::ScopedMachReceiveRight receive_right = AllocateReceiveRight();
  ASSERT_TRUE(receive_right.is_valid());
  base::apple::ScopedMachSendRight send_right =
      MakeSendRight(receive_right.get());

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

// The round trip both sides actually use: the server sends, the client reads,
// and the token names the sender. Both ends are this process here, so this
// covers the plumbing only.
TEST(IdentityChannelMacTest, ReadsAuditTokenFromSentMessage) {
  std::optional<IdentityChannel> channel = CreateIdentityChannel();
  ASSERT_TRUE(channel);

  SendIdentityMessage(std::move(channel->send_handle));

  const std::optional<audit_token_t> token =
      ReadIdentityMessage(std::move(channel->receive_handle));
  ASSERT_TRUE(token);
  EXPECT_EQ(getpid(), audit_token_to_pid(*token));
}

// The sender controls the message, so a body means this is not the message the
// contract describes: the trailer would sit somewhere else, and the token read
// from a fixed offset would be garbage.
TEST(IdentityChannelMacTest, RejectsMessageWithBody) {
  base::apple::ScopedMachReceiveRight receive_right = AllocateReceiveRight();
  base::apple::ScopedMachSendRight send_right =
      MakeSendRight(receive_right.get());

  struct {
    mach_msg_header_t header;
    uint32_t body;
  } message{};
  message.header.msgh_size = sizeof(message);
  message.body = 0xdeadbeef;
  SendRawMessage(send_right.get(), &message.header);

  EXPECT_FALSE(
      ReadIdentityMessage(mojo::PlatformHandle(std::move(receive_right))));
}

// A sender can attach rights in the message body. That is not the message the
// contract describes, so it is rejected rather than having its token read.
TEST(IdentityChannelMacTest, RejectsComplexMessage) {
  base::apple::ScopedMachReceiveRight receive_right = AllocateReceiveRight();
  base::apple::ScopedMachSendRight send_right =
      MakeSendRight(receive_right.get());

  // A second port, whose send right rides along on the message.
  base::apple::ScopedMachReceiveRight smuggled = AllocateReceiveRight();
  base::apple::ScopedMachSendRight smuggled_send =
      MakeSendRight(smuggled.get());

  struct {
    mach_msg_header_t header;
    mach_msg_body_t body;
    mach_msg_port_descriptor_t port;
  } message{};
  message.header.msgh_size = sizeof(message);
  message.header.msgh_bits = MACH_MSGH_BITS_COMPLEX;
  message.body.msgh_descriptor_count = 1;
  message.port.name = smuggled_send.get();
  message.port.disposition = MACH_MSG_TYPE_COPY_SEND;
  message.port.type = MACH_MSG_PORT_DESCRIPTOR;
  SendRawMessage(send_right.get(), &message.header);

  EXPECT_FALSE(
      ReadIdentityMessage(mojo::PlatformHandle(std::move(receive_right))));
}

}  // namespace brave_vpn::v2

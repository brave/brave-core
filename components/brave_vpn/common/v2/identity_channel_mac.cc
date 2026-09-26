/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/v2/identity_channel.h"

#include <mach/mach.h>

#include <utility>

#include "base/apple/mach_logging.h"
#include "base/apple/scoped_mach_port.h"
#include "base/check.h"
#include "base/logging.h"
#include "mojo/public/cpp/platform/platform_handle.h"

namespace brave_vpn::v2 {

std::optional<IdentityChannel> CreateIdentityChannel() {
  base::apple::ScopedMachReceiveRight receive_right;
  kern_return_t kr = mach_port_allocate(
      mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
      base::apple::ScopedMachReceiveRight::Receiver(receive_right).get());
  if (kr != KERN_SUCCESS) {
    MACH_LOG(WARNING, kr) << "identity channel port allocate";
    return std::nullopt;
  }

  // MAKE_SEND puts the send right under the same name as the receive right,
  // with one user reference. The server takes ownership of that reference and
  // releases it after sending; the receive right is untouched.
  kr = mach_port_insert_right(mach_task_self(), receive_right.get(),
                              receive_right.get(), MACH_MSG_TYPE_MAKE_SEND);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(WARNING, kr) << "identity channel insert send right";
    return std::nullopt;
  }

  base::apple::ScopedMachSendRight send_right{receive_right.get()};

  return IdentityChannel{
      .receive_handle = mojo::PlatformHandle(std::move(receive_right)),
      .send_handle = mojo::PlatformHandle(std::move(send_right))};
}

std::optional<audit_token_t> ReadIdentityMessage(
    mojo::PlatformHandle receive_handle) {
  // A moved-from handle means the caller already consumed the channel; a single
  // message means there is nothing to read a second time either way.
  if (!receive_handle.is_valid_mach_receive()) {
    return std::nullopt;
  }
  base::apple::ScopedMachReceiveRight receive_right =
      receive_handle.TakeMachReceiveRight();

  // The kernel appends the trailer immediately after the message, and this
  // message is header-only, so the trailer lands exactly on |message.trailer|.
  // Sized to the largest trailer format so any trailer the kernel offers fits.
  static_assert(
      round_msg(sizeof(mach_msg_header_t)) == sizeof(mach_msg_header_t),
      "a header-only message needs no trailer offset arithmetic");

  struct {
    mach_msg_header_t header;
    mach_msg_max_trailer_t trailer;
  } message{};

  const kern_return_t kr =
      mach_msg(&message.header,
               MACH_RCV_MSG | MACH_RCV_TIMEOUT |
                   MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0) |
                   MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AUDIT),
               /*send_size=*/0, sizeof(message), receive_right.get(),
               /*timeout=*/0, MACH_PORT_NULL);
  if (kr != KERN_SUCCESS) {
    // MACH_RCV_TIMED_OUT is the ordinary "server sent nothing" case.
    MACH_LOG_IF(WARNING, kr != MACH_RCV_TIMED_OUT, kr)
        << "identity message receive";
    return std::nullopt;
  }

  // The sender controls the message, not the trailer, so both are checked: a
  // body would move the trailer, and a short trailer means the kernel did not
  // fill in the audit token.
  if (message.header.msgh_size != sizeof(mach_msg_header_t) ||
      message.trailer.msgh_trailer_size < sizeof(mach_msg_audit_trailer_t)) {
    VLOG(1) << "identity message is not the expected shape";
    return std::nullopt;
  }

  return message.trailer.msgh_audit;
}

void SendIdentityMessage(mojo::PlatformHandle send_handle) {
  // Check if the handle contains a valid Mach send right. If it does not,
  // there is nothing to send on, which can happen if it did not survive
  // transit, or in unit tests.
  if (!send_handle.is_valid_mach_send()) {
    return;
  }

  base::apple::ScopedMachSendRight port = send_handle.TakeMachSendRight();

  // Mach message body is empty on purpose: the client reads the Mac audit
  // token appended by the kernel to the message trailer, which cannot be forged
  // by the server.
  mach_msg_header_t message{};
  message.msgh_bits = MACH_MSGH_BITS_REMOTE(MACH_MSG_TYPE_COPY_SEND);
  message.msgh_size = sizeof(message);
  message.msgh_remote_port = port.get();
  message.msgh_local_port = MACH_PORT_NULL;
  message.msgh_id = 0;  // Message id carries no meaning here.

  // The port is fresh and the client is about to receive one message on it, so
  // zero timeout is acceptable.
  const kern_return_t kr =
      mach_msg(&message, MACH_SEND_MSG | MACH_SEND_TIMEOUT, sizeof(message),
               /*rcv_size=*/0, MACH_PORT_NULL, /*timeout=*/0, MACH_PORT_NULL);
  MACH_LOG_IF(WARNING, kr != KERN_SUCCESS, kr) << "identity message send";
}

}  // namespace brave_vpn::v2

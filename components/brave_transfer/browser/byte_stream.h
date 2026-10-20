// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_BYTE_STREAM_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_BYTE_STREAM_H_

#include <cstddef>
#include <cstdint>

#include "base/containers/span.h"
#include "base/functional/callback.h"

namespace brave_transfer {

// A connected, reliable, ordered byte stream, such as a TCP connection.
// Destroying the stream closes it in both directions.
class ByteStream {
 public:
  using ReadCallback = base::RepeatingCallback<void(base::span<const uint8_t>)>;
  // Runs once, when the remote side has finished sending (`error` false) or
  // the stream failed (`error` true). No reads follow.
  using ClosedCallback = base::OnceCallback<void(bool error)>;

  virtual ~ByteStream() = default;

  // Starts delivering received data. Callbacks never run synchronously from
  // inside a ByteStream method.
  virtual void Start(ReadCallback on_read, ClosedCallback on_closed) = 0;

  // Queues `data` for sending.
  virtual void Write(base::span<const uint8_t> data) = 0;
  // Bytes queued but not yet handed to the network.
  virtual size_t BufferedBytes() const = 0;
  // Runs whenever BufferedBytes() decreases.
  virtual void SetWritableCallback(base::RepeatingClosure on_writable) = 0;

  // Sends the queued data, then ends the sending direction.
  virtual void CloseWrite() = 0;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_BYTE_STREAM_H_

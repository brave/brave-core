// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TEST_FAKE_BYTE_STREAM_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TEST_FAKE_BYTE_STREAM_H_

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_transfer/browser/byte_stream.h"

namespace brave_transfer {

// One end of an in-memory connection. Data and close notifications reach the
// other end in posted tasks.
class FakeByteStream : public ByteStream {
 public:
  static std::pair<std::unique_ptr<FakeByteStream>,
                   std::unique_ptr<FakeByteStream>>
  CreatePair();

  FakeByteStream();
  FakeByteStream(const FakeByteStream&) = delete;
  FakeByteStream& operator=(const FakeByteStream&) = delete;
  ~FakeByteStream() override;

  // ByteStream:
  void Start(ReadCallback on_read, ClosedCallback on_closed) override;
  void Write(base::span<const uint8_t> data) override;
  size_t BufferedBytes() const override;
  void SetWritableCallback(base::RepeatingClosure on_writable) override;
  void CloseWrite() override;

  // Fails both ends.
  void SimulateError();
  size_t bytes_written() const { return bytes_written_; }
  base::WeakPtr<FakeByteStream> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  void Deliver(std::vector<uint8_t> data);
  void OnRemoteClosed(bool error);
  void Flush();

  base::WeakPtr<FakeByteStream> peer_;
  ReadCallback on_read_;
  ClosedCallback on_closed_;
  bool started_ = false;
  bool write_closed_ = false;
  std::deque<std::vector<uint8_t>> pending_;
  std::optional<bool> pending_close_;
  size_t bytes_written_ = 0;
  base::WeakPtrFactory<FakeByteStream> weak_factory_{this};
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TEST_FAKE_BYTE_STREAM_H_

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_TRANSFER_MOJO_BYTE_STREAM_H_
#define BRAVE_BROWSER_BRAVE_TRANSFER_MOJO_BYTE_STREAM_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_transfer/browser/byte_stream.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"

namespace brave_transfer {

// A ByteStream over a network service TCP socket and its data pipes.
class MojoByteStream : public ByteStream,
                       public network::mojom::SocketObserver {
 public:
  static constexpr size_t kMaxReadSize = 64 * 1024;

  MojoByteStream(mojo::PendingRemote<network::mojom::TCPConnectedSocket> socket,
                 mojo::PendingReceiver<network::mojom::SocketObserver> observer,
                 mojo::ScopedDataPipeConsumerHandle receive_stream,
                 mojo::ScopedDataPipeProducerHandle send_stream);
  MojoByteStream(const MojoByteStream&) = delete;
  MojoByteStream& operator=(const MojoByteStream&) = delete;
  ~MojoByteStream() override;

  // ByteStream:
  void Start(ReadCallback on_read, ClosedCallback on_closed) override;
  void Write(base::span<const uint8_t> data) override;
  size_t BufferedBytes() const override;
  void SetWritableCallback(base::RepeatingClosure on_writable) override;
  void CloseWrite() override;

 private:
  // network::mojom::SocketObserver:
  void OnReadError(int32_t net_error) override;
  void OnWriteError(int32_t net_error) override;

  void OnReadable(MojoResult result);
  void OnWritable(MojoResult result);
  // Returns false if the send pipe failed.
  bool Flush();
  void MaybeFinishWrite();
  // Ends reading. After a graceful end the sending direction stays usable.
  void Finish(bool error);
  void RunClosedCallback();

  mojo::Remote<network::mojom::TCPConnectedSocket> socket_;
  mojo::Receiver<network::mojom::SocketObserver> observer_receiver_{this};
  mojo::ScopedDataPipeConsumerHandle receive_stream_;
  mojo::ScopedDataPipeProducerHandle send_stream_;
  mojo::SimpleWatcher readable_watcher_;
  mojo::SimpleWatcher writable_watcher_;

  ReadCallback on_read_;
  ClosedCallback on_closed_;
  base::RepeatingClosure on_writable_;

  std::vector<uint8_t> pending_;
  size_t pending_offset_ = 0;
  bool close_write_requested_ = false;
  bool finished_ = false;
  bool closed_with_error_ = false;
  bool send_failed_ = false;

  base::WeakPtrFactory<MojoByteStream> weak_factory_{this};
};

}  // namespace brave_transfer

#endif  // BRAVE_BROWSER_BRAVE_TRANSFER_MOJO_BYTE_STREAM_H_

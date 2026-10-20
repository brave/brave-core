// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation

public protocol ByteStreamDelegate: AnyObject {
  func byteStream(_ stream: ByteStream, didRead data: Bytes)
  /// `pendingWriteBytes` went down.
  func byteStreamDidDrain(_ stream: ByteStream)
  /// The peer finished sending (`error` nil) or the stream failed. Runs once;
  /// no reads follow.
  func byteStream(_ stream: ByteStream, didCloseWithError error: Error?)
}

/// A connected, reliable, ordered byte stream, such as a TCP connection.
/// Callbacks run on the owner's scheduler and never synchronously from inside
/// a ByteStream method.
public protocol ByteStream: AnyObject {
  var delegate: ByteStreamDelegate? { get set }
  /// Starts delivering received data.
  func start()
  /// Queues `data` for sending.
  func write(_ data: Bytes)
  /// Bytes queued but not yet handed to the network.
  var pendingWriteBytes: Int { get }
  /// Closes both directions. No callbacks follow.
  func close()
}

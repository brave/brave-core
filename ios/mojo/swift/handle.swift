// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

internal import BraveMojoSystem
import Foundation
import Synchronization

/// An owned mojo handle, closed when the last reference to it goes away.
///
/// Encoding a handle into a message transfers it, after which this instance is
/// invalid. Copies of a value holding a handle (e.g. a ``PendingRemote``) share
/// the same instance, so sending one copy invalidates all of them.
public final class MojoHandle: Sendable {
  private let value: Mutex<UInt>

  /// Adopts `rawValue`, which must be an open handle not owned by anything else.
  init(adopting rawValue: UInt) {
    value = Mutex(rawValue)
  }

  deinit {
    // Mojo system calls other than watching may be made from any thread.
    BraveMojoClose(value.withLock { $0 })
  }

  /// Whether this instance still owns a handle.
  public var isValid: Bool {
    value.withLock { $0 != 0 }
  }

  /// Releases ownership of the handle, returning it, or 0 if it was already
  /// released.
  func take() -> UInt {
    value.withLock { value in
      let handle = value
      value = 0
      return handle
    }
  }

  /// The raw handle for making system calls on it while this instance stays
  /// alive. Returns 0 if the handle was already released.
  var rawValue: UInt {
    value.withLock { $0 }
  }

  /// Closes the handle now rather than when this instance is released.
  public func close() {
    BraveMojoClose(take())
  }
}

/// The two ends of a message pipe.
struct MessagePipe: Sendable {
  let handle0: MojoHandle
  let handle1: MojoHandle

  init() throws {
    var handle0: BraveMojoHandle = 0
    var handle1: BraveMojoHandle = 0
    let result = BraveMojoCreateMessagePipe(&handle0, &handle1)
    guard result == .ok else {
      throw MojoSystemError(result)
    }
    self.handle0 = MojoHandle(adopting: handle0)
    self.handle1 = MojoHandle(adopting: handle1)
  }
}

/// A failed mojo system call.
struct MojoSystemError: Error, Equatable {
  /// The `MojoResult` the call returned.
  let result: UInt32

  init(_ result: BraveMojoResult) {
    self.result = result.rawValue
  }

  /// Returned when reading from a pipe that has no messages yet.
  static var shouldWait: MojoSystemError { MojoSystemError(.shouldWait) }

  /// Returned when the pipe's peer has been closed.
  static var failedPrecondition: MojoSystemError { MojoSystemError(.failedPrecondition) }
}

extension MojoHandle {
  /// Writes `message` to this message pipe, transferring its handles.
  func writeMessage(_ message: consuming Message) throws {
    let handles = message.handles.map { NSNumber(value: $0.take()) }
    let result = BraveMojoWriteMessage(rawValue, Data(message.bytes), handles)
    guard result == .ok else {
      throw MojoSystemError(result)
    }
  }

  /// Reads the next message from this message pipe.
  func readMessage() throws -> Message {
    var message = Message()
    let result = BraveMojoReadMessage(rawValue) { bytes, handles in
      message.bytes = [UInt8](bytes)
      message.handles = handles.map { MojoHandle(adopting: $0.uintValue) }
    }
    guard result == .ok else {
      throw MojoSystemError(result)
    }
    return message
  }
}

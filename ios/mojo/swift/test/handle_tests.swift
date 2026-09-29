// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import SwiftMojo

private enum PipeInterface: MojomInterface {
  static let name = "swift_mojo_test.mojom.PipeInterface"
  static let version: UInt32 = 3
}

// struct Endpoints {
//   pending_remote<PipeInterface> remote;
//   pending_receiver<PipeInterface> receiver;
// };
private struct Endpoints: MojomStruct {
  var remote: PendingRemote<PipeInterface>
  var receiver: PendingReceiver<PipeInterface>

  static let versions = [MojomStructVersion(version: 0, numBytes: 24)]

  init(remote: PendingRemote<PipeInterface>, receiver: PendingReceiver<PipeInterface>) {
    self.remote = remote
    self.receiver = receiver
  }

  func encodeFields(to encoder: inout MojoEncoder, at offset: Int) {
    remote.encode(to: &encoder, at: offset + 8)
    receiver.encode(to: &encoder, at: offset + 16)
  }

  init(fields decoder: inout MojoDecoder, at offset: Int, version: UInt32) throws {
    remote = try PendingRemote(from: &decoder, at: offset + 8)
    receiver = try PendingReceiver(from: &decoder, at: offset + 16)
  }
}

private func assertThrows(
  _ expected: MojoSystemError,
  file: StaticString = #filePath,
  line: UInt = #line,
  _ body: () throws -> Void
) {
  do {
    try body()
    XCTFail("Expected \(expected) to be thrown", file: file, line: line)
  } catch {
    XCTAssertEqual(error as? MojoSystemError, expected, file: file, line: line)
  }
}

final class HandleTests: XCTestCase {
  func testMojoIsInitialized() {
    XCTAssertTrue(MojoSystem.isInitialized)
  }

  func testWriteReadRoundTrip() throws {
    let pipe = try MessagePipe()
    try pipe.handle0.writeMessage(Message(bytes: [1, 2, 3]))
    try pipe.handle0.writeMessage(Message(bytes: []))

    XCTAssertEqual(try pipe.handle1.readMessage().bytes, [1, 2, 3])
    let empty = try pipe.handle1.readMessage()
    XCTAssertEqual(empty.bytes, [])
    XCTAssertTrue(empty.handles.isEmpty)
  }

  func testReadEmptyPipeShouldWait() throws {
    let pipe = try MessagePipe()
    assertThrows(.shouldWait) { _ = try pipe.handle1.readMessage() }
  }

  func testPeerClosed() throws {
    let pipe = try MessagePipe()
    try pipe.handle0.writeMessage(Message(bytes: [1]))
    pipe.handle0.close()
    XCTAssertFalse(pipe.handle0.isValid)

    // Messages written before the peer was closed are still delivered.
    XCTAssertEqual(try pipe.handle1.readMessage().bytes, [1])
    assertThrows(.failedPrecondition) { _ = try pipe.handle1.readMessage() }
    assertThrows(.failedPrecondition) {
      try pipe.handle1.writeMessage(Message(bytes: [1]))
    }
  }

  func testTransfersHandles() throws {
    let pipe = try MessagePipe()
    let other = try MessagePipe()
    try pipe.handle0.writeMessage(Message(bytes: [7], handles: [other.handle1]))
    XCTAssertFalse(other.handle1.isValid)

    let received = try pipe.handle1.readMessage()
    XCTAssertEqual(received.bytes, [7])
    XCTAssertEqual(received.handles.count, 1)

    // The received handle is the other end of `other.handle0`.
    try other.handle0.writeMessage(Message(bytes: [8, 9]))
    XCTAssertEqual(try received.handles[0].readMessage().bytes, [8, 9])
  }

  func testFailedWriteClosesHandles() throws {
    let pipe = try MessagePipe()
    let other = try MessagePipe()
    pipe.handle1.close()
    assertThrows(.failedPrecondition) {
      try pipe.handle0.writeMessage(Message(bytes: [], handles: [other.handle1]))
    }
    XCTAssertFalse(other.handle1.isValid)
    assertThrows(.failedPrecondition) {
      try other.handle0.writeMessage(Message(bytes: [1]))
    }
  }

  func testDeinitClosesHandle() throws {
    let handle0: MojoHandle
    do {
      let pipe = try MessagePipe()
      handle0 = pipe.handle0
    }
    assertThrows(.failedPrecondition) {
      try handle0.writeMessage(Message(bytes: [1]))
    }
  }

  func testEncodesPendingEndpoints() throws {
    let (remote, receiver) = try PendingRemote<PipeInterface>.makePipe()
    let message = Message(
      header: MessageHeader(name: 1),
      payload: Endpoints(remote: remote, receiver: receiver)
    )
    // Encoding transfers the pipes into the message.
    XCTAssertFalse(remote.isValid)
    XCTAssertFalse(receiver.isValid)
    XCTAssertEqual(message.handles.count, 2)

    let pipe = try MessagePipe()
    try pipe.handle0.writeMessage(message)
    var received = try pipe.handle1.readMessage()
    let header = try received.parseHeader()
    let endpoints = try received.decodePayload(Endpoints.self, parsedHeader: header)
    XCTAssertEqual(endpoints.remote.version, PipeInterface.version)

    // The decoded endpoints are still connected to each other.
    try endpoints.remote.handle.writeMessage(Message(bytes: [4, 2]))
    XCTAssertEqual(try endpoints.receiver.handle.readMessage().bytes, [4, 2])
  }
}

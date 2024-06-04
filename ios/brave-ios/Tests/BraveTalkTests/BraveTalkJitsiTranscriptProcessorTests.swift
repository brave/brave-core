// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Collections
import Foundation
@_spi(AppLaunch) import Shared
import TestHelpers
import XCTest

@testable import BraveTalk

class BraveTalkJitsiTranscriptProcessorTests: XCTestCase {
  var transcriptProcessor: BraveTalkJitsiTranscriptProcessor!

  override func setUpWithError() throws {
    transcriptProcessor = BraveTalkJitsiTranscriptProcessor()
  }

  func testSingleMessageProcessing() async throws {
    let dictionary: [AnyHashable: Any] = [
      "participant": ["id": "1", "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["unstable": "Hello there."],
      "language": "en",
      "messageID": "testMessageID1",
    ]

    await transcriptProcessor.processTranscript(dictionary: dictionary)
    let transcript = try await XCTunwrapAsync(await self.transcriptProcessor.getTranscript())

    XCTAssertTrue(
      transcript.contains("Alice: Hello there."),
      "The transcript text should match the unstable text initially provided."
    )
  }

  func testUpdatingSameMessageID() async throws {
    let initialDict: [AnyHashable: Any] = [
      "participant": ["id": "1", "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["unstable": "Hello there."],
      "language": "en",
      "messageID": "testMessageID1",
    ]

    let updatedDict: [AnyHashable: Any] = [
      "participant": ["id": "1", "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["stable": "Hello there, how are you?"],
      "language": "en",
      "messageID": "testMessageID1",
    ]

    let finalDict: [AnyHashable: Any] = [
      "participant": ["id": "1", "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["final": "Goodbye."],
      "language": "en",
      "messageID": "testMessageID1",
    ]

    await transcriptProcessor.processTranscript(dictionary: initialDict)
    await transcriptProcessor.processTranscript(dictionary: updatedDict)
    await transcriptProcessor.processTranscript(dictionary: finalDict)

    let transcript = try await XCTunwrapAsync(await self.transcriptProcessor.getTranscript())

    XCTAssertTrue(
      transcript.contains("Alice: Goodbye."),
      "The transcript text should prioritize 'final' over 'stable' and 'unstable'."
    )

    XCTAssertFalse(
      transcript.contains("how are you"),
      "The transcript text should prioritize 'final' over 'stable' and 'unstable'."
    )

    XCTAssertFalse(
      transcript.contains("Hello there."),
      "The transcript text should prioritize 'final' over 'stable' and 'unstable'."
    )
  }

  func testTextPriority() async throws {
    let mixedPriorityDict: [AnyHashable: Any] = [
      "participant": ["id": "1", "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["unstable": "First text", "stable": "Second text", "final": "Final text"],
      "language": "en",
      "messageID": "testMessageID2",
    ]

    await transcriptProcessor.processTranscript(dictionary: mixedPriorityDict)
    let transcript = try await XCTunwrapAsync(await self.transcriptProcessor.getTranscript())

    XCTAssertTrue(
      transcript.contains("Alice: Final text"),
      "The transcript text should prioritize 'final' over 'stable' and 'unstable'."
    )
  }
}

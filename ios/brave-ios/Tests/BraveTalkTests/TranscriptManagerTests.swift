// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Collections
import Foundation
@_spi(AppLaunch) import Shared
import XCTest

@testable import BraveTalk

class TranscriptManagerTests: XCTestCase {

  override func setUpWithError() throws {
    TranscriptManager.transcripts.removeAll()
  }

  func testSingleMessageProcessing() async throws {
    let dictionary: [AnyHashable: Any] = [
      "participant": ["id": 1, "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["unstable": "Hello there."],
      "language": "en",
      "messageID": "testMessageID1",
    ]

    await TranscriptManager.processMessage(dictionary: dictionary)

    let transcript = await TranscriptManager.generateTranscript()
    XCTAssertTrue(
      transcript.contains("Alice: Hello there."),
      "The transcript text should match the unstable text initially provided."
    )
  }

  func testUpdatingSameMessageID() async throws {
    let initialDict: [AnyHashable: Any] = [
      "participant": ["id": 1, "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["unstable": "Hello there."],
      "language": "en",
      "messageID": "testMessageID1",
    ]

    let updatedDict: [AnyHashable: Any] = [
      "participant": ["id": 1, "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["stable": "Hello there, how are you?"],
      "language": "en",
      "messageID": "testMessageID1",
    ]

    let finalDict: [AnyHashable: Any] = [
      "participant": ["id": 1, "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["final": "Goodbye."],
      "language": "en",
      "messageID": "testMessageID1",
    ]

    await TranscriptManager.processMessage(dictionary: initialDict)
    await TranscriptManager.processMessage(dictionary: updatedDict)
    await TranscriptManager.processMessage(dictionary: finalDict)

    let transcript = await TranscriptManager.generateTranscript()
    XCTAssertEqual(
      transcript.first,
      "Alice: Goodbye.",
      "The transcript text should match the final text provided."
    )
  }

  func testTextPriority() async throws {
    let mixedPriorityDict: [AnyHashable: Any] = [
      "participant": ["id": 1, "name": "Alice", "avatarUrl": "http://example.com/avatar.png"],
      "text": ["unstable": "First text", "stable": "Second text", "final": "Final text"],
      "language": "en",
      "messageID": "testMessageID2",
    ]

    await TranscriptManager.processMessage(dictionary: mixedPriorityDict)

    let transcript = await TranscriptManager.generateTranscript()
    XCTAssertTrue(
      transcript.contains("Alice: Final text"),
      "The transcript text should prioritize 'final' over 'stable' and 'unstable'."
    )
  }
}

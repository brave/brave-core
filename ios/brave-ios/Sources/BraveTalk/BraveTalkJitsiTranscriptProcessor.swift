// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Collections
import Foundation
import os.log

public class BraveTalkJitsiTranscriptProcessor {
  var transcripts: OrderedDictionary<String, Transcript> = OrderedDictionary()

  struct Participant: Codable {
    var id: String
    var name: String
    var avatarUrl: String?
  }

  struct Transcript: Codable {
    var participant: Participant
    var text: Text
    var language: String
    var messageID: String

    struct Text: Codable {
      var final: String?
      var stable: String?
      var unstable: String?

      var bestText: String {
        final ?? stable ?? unstable ?? ""
      }
    }
  }

  @MainActor
  public func getTranscript() async -> String? {
    if transcripts.isEmpty {
      return nil
    }

    return transcripts.map { "\($0.value.participant.name): \($0.value.text.bestText)" }.joined(
      separator: "\n"
    )
  }

  @MainActor
  func processTranscript(dictionary: [AnyHashable: Any]) async {
    let decoder = JSONDecoder()
    do {
      let data = try JSONSerialization.data(withJSONObject: dictionary, options: [])
      let newTranscript = try decoder.decode(Transcript.self, from: data)

      await updateTranscripts(newTranscript: newTranscript)
    } catch {
      Logger.module.error("Failed to decode or process the message: \(error)")
    }
  }

  @MainActor
  private func updateTranscripts(newTranscript: Transcript) async {
    if var existingTranscript = transcripts[newTranscript.messageID] {
      existingTranscript.text.final = newTranscript.text.final ?? existingTranscript.text.final
      existingTranscript.text.stable = newTranscript.text.stable ?? existingTranscript.text.stable
      existingTranscript.text.unstable =
        newTranscript.text.unstable ?? existingTranscript.text.unstable
      transcripts[newTranscript.messageID] = existingTranscript
    } else {
      transcripts[newTranscript.messageID] = newTranscript
    }
  }
}

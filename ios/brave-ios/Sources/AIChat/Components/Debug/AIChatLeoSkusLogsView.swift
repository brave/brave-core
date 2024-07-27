// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStore
import BraveUI
import OSLog
import Preferences
import SwiftUI
import os.log

public struct AIChatLeoSkusLogsView: View {
  @State
  private var text = ""

  public init() {

  }

  public var body: some View {
    ScrollView {
      Text(text)
        .font(.caption)
        .fixedSize(horizontal: false, vertical: true)
        .frame(maxWidth: .infinity)
        .padding()
    }
    .task {
      text = "Loading. Please Wait..."
      text = await fetchLogs()
    }
    .navigationTitle("Leo Skus Logs")
    .toolbar {
      Button("Copy") {
        UIPasteboard.general.string = text
      }
    }
  }

  private func fetchLogs() async -> String {
    do {
      let formatter = DateFormatter()
      formatter.dateStyle = .short
      formatter.timeStyle = .short

      let store = try OSLogStore(scope: .currentProcessIdentifier)
      let logs = try store.getEntries()
        .compactMap { $0 as? OSLogEntryLog }
        .filter { $0.category == "AIChat" && $0.subsystem == Bundle.main.bundleIdentifier }
        .map { "\(formatter.string(from: $0.date)): \($0.composedMessage)" }
        .joined(separator: "\n----------------------\n")

      return logs + "\n\n ---- Skus Info ----\n\n" + (await getSkusState())
    } catch {
      return "Error Fetching Logs: \(error)"
    }
  }

  @MainActor
  private func getSkusState() async -> String {
    var result = ""
    let orderId = Preferences.AIChat.subscriptionOrderId.value ?? "None"

    result += "OrderId: \(orderId)\n"

    do {
      let credentials = try await BraveSkusSDK.shared.credentialsSummary(for: .leo)
      if let jsonData = try? jsonEncoder.encode(credentials),
        let credentialsJSON = String(data: jsonData, encoding: .utf8)
      {
        result += "Credentials: \(credentialsJSON)\n"
      } else {
        result += "Credentials: \(credentials)\n"
      }
    } catch {
      result += "Credentials Error: \(error)\n"
    }

    return result
  }

  /// A custom JSON Encoder that handles encoding Skus Object dates as ISO-8601
  /// with optional milli-seconds
  private let jsonEncoder: JSONEncoder = {
    let formatter = ISO8601DateFormatter()
    formatter.formatOptions = [
      .withYear,
      .withMonth,
      .withDay,
      .withTime,
      .withDashSeparatorInDate,
      .withColonSeparatorInTime,
    ]

    let encoder = JSONEncoder()
    encoder.outputFormatting = [.prettyPrinted, .withoutEscapingSlashes, .sortedKeys]
    encoder.keyEncodingStrategy = .convertToSnakeCase
    encoder.dateEncodingStrategy = .custom({ date, encoder in
      var container = encoder.singleValueContainer()
      try container.encode(formatter.string(from: date))
    })
    return encoder
  }()
}

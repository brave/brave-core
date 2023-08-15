// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveUI
import BraveCore
import OSLog
import os.log

fileprivate class LogLineCell: UITableViewCell, TableViewReusable {
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
    textLabel?.font = .systemFont(ofSize: 12, weight: .regular)
    textLabel?.numberOfLines = 0
    detailTextLabel?.font = .systemFont(ofSize: 14, weight: .semibold)
    detailTextLabel?.numberOfLines = 0
    selectionStyle = .none
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

/// A file generator that copies all Rewards related log files into the sharable directory
struct RewardsInternalsLogsGenerator: RewardsInternalsFileGenerator {
  func generateFiles(at path: String,
                     using builder: RewardsInternalsSharableBuilder,
                     completion: @escaping (Error?) -> Void) {
    
    do {
      let tempURL = FileManager.default.temporaryDirectory.appendingPathComponent("rewards_log.txt")
      let formatter = DateFormatter()
      formatter.dateStyle = .short
      formatter.timeStyle = .short
      
      let store = try OSLogStore(scope: .currentProcessIdentifier)
      let logs = try store.getEntries()
        .compactMap { $0 as? OSLogEntryLog }
        .filter { $0.category == "ads-rewards" && $0.subsystem == Bundle.main.bundleIdentifier }
        .map { "\(formatter.string(from: $0.date)): \($0.composedMessage)" }
        .joined(separator: "\n")
      
      try logs.write(toFile: tempURL.path, atomically: true, encoding: .utf8)
      let logPath = URL(fileURLWithPath: path).appendingPathComponent(tempURL.lastPathComponent)
      try FileManager.default.copyItem(atPath: tempURL.path, toPath: logPath.path)
      try FileManager.default.removeItem(atPath: tempURL.path)
      
      completion(nil)
    } catch {
      completion(error)
    }
  }
}


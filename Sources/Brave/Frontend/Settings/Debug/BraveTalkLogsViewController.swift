// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import os.log
import OSLog
import SnapKit

public class BraveTalkLogsViewController: UIViewController {
  
  private var logsTextView = UITextView()
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    
    logsTextView.isEditable = false
    view.addSubview(logsTextView)
    logsTextView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    
    logsTextView.text = getLogs()
    
    let shareButton = UIBarButtonItem(barButtonSystemItem: .action, target: self, action: #selector(shareLogs))
    navigationItem.rightBarButtonItem = shareButton
  }
  
  @objc private func shareLogs() {
    do {
      let fileURL = try createTemporaryLogFile()
      let activityViewController = UIActivityViewController(activityItems: [fileURL], applicationActivities: nil)
      present(activityViewController, animated: true)
    } catch {
      Logger.module.error("Error while creating the log file: \(error.localizedDescription, privacy: .public)")
    }
  }
  
  private func createTemporaryLogFile() throws -> URL {
    let logsString = getLogs()
    let temporaryDirectoryURL = FileManager.default.temporaryDirectory
    let logFileURL = temporaryDirectoryURL.appendingPathComponent("Logs.txt")
    
    try logsString.write(to: logFileURL, atomically: true, encoding: .utf8)
    return logFileURL
  }
  
  private func getLogs() -> String {
    do {
      let store = try OSLogStore(scope: .currentProcessIdentifier)
      
      return try store
        .getEntries()
        .compactMap { $0 as? OSLogEntryLog }
        .filter { $0.category == "BraveTalk" && $0.subsystem == Bundle.main.bundleIdentifier }
        .map { "[\($0.date.formatted())] \($0.composedMessage)" }
        .joined(separator: "\n")
    } catch {
      Logger.module.error("\(error.localizedDescription, privacy: .public)")
    }
    
    return ""
  }
}

/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit

public struct UrpLog {
  static let prefsKey = "urpLogs"

  public static func log(_ text: String) {
    var logs = UserDefaults.standard.string(forKey: prefsKey) ?? ""

    let date = Date()
    let calendar = Calendar.current
    let components = calendar.dateComponents([.year, .month, .day, .hour, .minute], from: date)

    guard let year = components.year, let month = components.month, let day = components.day, let hour = components.hour, let minute = components.minute else {
      return
    }

    let time = "\(year)-\(month)-\(day) \(hour):\(minute)"
    logs.append("[\(time)] \(text)\n")

    UserDefaults.standard.set(logs, forKey: prefsKey)
  }
}

public class UrpLogsViewController: UIViewController {
  lazy var logsTextView: UITextView = {
    let textView = UITextView()
    textView.isEditable = false
    return textView
  }()

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.addSubview(logsTextView)
    logsTextView.snp.makeConstraints { make in
      make.top.equalTo(self.view.safeAreaLayoutGuide.snp.top).offset(8)
      make.left.right.bottom.equalTo(self.view).inset(8)
    }

    guard let logs = UserDefaults.standard.string(forKey: UrpLog.prefsKey) else { return }
    logsTextView.text = logs
  }
}

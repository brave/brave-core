// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Preferences
import Shared
import UIKit

public enum LoggerType {
  case secureState
  case userAgent

  var prefsKey: String {
    switch self {
    case .secureState:
      return "secureStateLogs"
    case .userAgent:
      return "userAgentLogs"
    }
  }

  var loggingEnabledKey: String {
    return "\(prefsKey)Enabled"
  }

  var isLoggingEnabled: Bool {
    get {
      // Default to disabled
      if UserDefaults.standard.object(forKey: loggingEnabledKey) == nil {
        return false
      }
      return UserDefaults.standard.bool(forKey: loggingEnabledKey)
    }
    set {
      UserDefaults.standard.set(newValue, forKey: loggingEnabledKey)
    }
  }
}

public struct DebugLogger {
  public static func log(for type: LoggerType, text: String) {
    guard type.isLoggingEnabled else { return }

    var logs = UserDefaults.standard.string(forKey: type.prefsKey) ?? ""

    let date = Date()
    let calendar = Calendar.current
    let components = calendar.dateComponents([.year, .month, .day, .hour, .minute], from: date)

    guard let year = components.year,
      let month = components.month,
      let day = components.day,
      let hour = components.hour,
      let minute = components.minute
    else {
      return
    }
    let time = "\(year)-\(month)-\(day) \(hour):\(minute)"

    switch type {
    case .secureState, .userAgent:
      logs.append("------------------------------------\n\n")
      logs.append(" [\(time)]\n\n \(text)\n")
    }

    UserDefaults.standard.set(logs, forKey: type.prefsKey)
  }

  public static func cleanLogger(for type: LoggerType) {
    UserDefaults.standard.removeObject(forKey: type.prefsKey)
  }

  public static func cleanUrpLogs() {
    // Delete the no-longer used urp logs if they exist
    UserDefaults.standard.removeObject(forKey: "urpLogs")
  }
}

public class DebugLogViewController: UIViewController {
  var loggerType: LoggerType

  public init(type: LoggerType) {
    loggerType = type

    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    switch loggerType {
    case .secureState:
      title = "Secure Content State"
    case .userAgent:
      title = "User Agent Debug"
    }

    let shareBarButtonItem = UIBarButtonItem(
      barButtonSystemItem: .action,
      target: self,
      action: #selector(shareButtonTapped)
    )

    let loggingSwitch = UISwitch()
    loggingSwitch.isOn = loggerType.isLoggingEnabled
    loggingSwitch.addTarget(self, action: #selector(loggingSwitchToggled(_:)), for: .valueChanged)
    let switchBarButtonItem = UIBarButtonItem(customView: loggingSwitch)

    navigationItem.rightBarButtonItems = [shareBarButtonItem, switchBarButtonItem]

    let textView = UITextView()
    textView.translatesAutoresizingMaskIntoConstraints = false
    textView.isEditable = false

    view.addSubview(textView)

    NSLayoutConstraint.activate([
      textView.topAnchor.constraint(
        equalTo: view.topAnchor,
        constant: 8
      ),
      textView.leadingAnchor.constraint(
        equalTo: view.leadingAnchor,
        constant: 8
      ),
      textView.trailingAnchor.constraint(
        equalTo: view.trailingAnchor,
        constant: -8
      ),
      textView.bottomAnchor.constraint(
        equalTo: view.bottomAnchor,
        constant: -8
      ),
    ])

    guard let logs = UserDefaults.standard.string(forKey: loggerType.prefsKey) else { return }
    let title = "\(title ?? "") Logs\n\n"

    textView.text = title + logs
  }

  @objc func loggingSwitchToggled(_ sender: UISwitch) {
    loggerType.isLoggingEnabled = sender.isOn
  }

  @objc func shareButtonTapped() {
    guard let logs = UserDefaults.standard.string(forKey: loggerType.prefsKey) else { return }

    // Create an activity view controller with the text to share
    let activityViewController = UIActivityViewController(
      activityItems: [logs],
      applicationActivities: nil
    )

    // Present the activity view controller
    if let popoverController = activityViewController.popoverPresentationController {
      popoverController.barButtonItem = navigationItem.rightBarButtonItems?.first
    }
    present(activityViewController, animated: true, completion: nil)
  }
}

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Foundation
import UIKit

/// One foreground idle-timer assertion for all active Tor downloads, across windows.
@MainActor
final class TorDownloadActivity {
  static let shared = TorDownloadActivity()
  private var downloads: Set<UUID> = []
  private var previousIdleTimerValue: Bool?
  private let lowPowerMode: @MainActor () -> Bool
  private let foreground: @MainActor () -> Bool
  private let readIdleTimer: @MainActor () -> Bool
  private let writeIdleTimer: @MainActor (Bool) -> Void
  private let notifications: NotificationCenter
  private var observations: [NSObjectProtocol] = []

  init(
    lowPowerMode: @escaping @MainActor () -> Bool = { ProcessInfo.processInfo.isLowPowerModeEnabled },
    foreground: @escaping @MainActor () -> Bool = { UIApplication.shared.applicationState == .active },
    readIdleTimer: @escaping @MainActor () -> Bool = { UIApplication.shared.isIdleTimerDisabled },
    writeIdleTimer: @escaping @MainActor (Bool) -> Void = { UIApplication.shared.isIdleTimerDisabled = $0 },
    notifications: NotificationCenter = .default
  ) {
    self.lowPowerMode = lowPowerMode
    self.foreground = foreground
    self.readIdleTimer = readIdleTimer
    self.writeIdleTimer = writeIdleTimer
    self.notifications = notifications
    for name in [Notification.Name.NSProcessInfoPowerStateDidChange,
                 UIApplication.didBecomeActiveNotification, UIApplication.didEnterBackgroundNotification] {
      observations.append(notifications.addObserver(forName: name, object: nil, queue: .main) {
        [weak self] _ in
        Task { @MainActor [weak self] in self?.refresh() }
      })
    }
  }

  isolated deinit {
    observations.forEach { notifications.removeObserver($0) }
    if let previousIdleTimerValue { writeIdleTimer(previousIdleTimerValue) }
  }

  func begin(_ id: UUID) { downloads.insert(id); refresh() }
  func end(_ id: UUID) { downloads.remove(id); refresh() }

  func refresh() {
    let shouldStayAwake = !downloads.isEmpty && foreground() && !lowPowerMode()
    if shouldStayAwake {
      if previousIdleTimerValue == nil { previousIdleTimerValue = readIdleTimer() }
      writeIdleTimer(true)
    } else if let previous = previousIdleTimerValue {
      previousIdleTimerValue = nil
      writeIdleTimer(previous)
    }
  }
}

// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import UIKit

@Observable
class DefaultBrowserHelper {

  enum Status {
    case unknown
    case notDefaulted
    case likely
    case defaulted
  }

  private(set) var status: Status = .unknown

  @ObservationIgnored
  private let isDefaultAppChecker: IsDefaultAppChecker

  @ObservationIgnored
  var now: () -> Date = { .now }

  init(
    isDefaultAppChecker: IsDefaultAppChecker = .live
  ) {
    self.isDefaultAppChecker = isDefaultAppChecker
    updateStatus()
  }

  var isAccurateDefaultCheckAvailable: Bool {
    isDefaultAppChecker.isAvailable()
  }

  /// Whether or not an accurate default check should be performed
  var shouldPerformAccurateDefaultCheck: Bool {
    // Only use new logic if API is available
    guard isDefaultAppChecker.isAvailable() else {
      return false
    }

    guard let installDate = Preferences.DAU.appRetentionLaunchDate.value else { return false }

    // Check if we should show a prompt today based on schedule and count
    let daysSinceInstall =
      Calendar.current.dateComponents([.day], from: installDate, to: now()).day ?? 0

    // Determine current prompt window
    let currentWindow = DefaultCheckWindow(daysSinceInstall: daysSinceInstall)
    guard let window = currentWindow else { return false }

    // Check if we've already checked if for this window
    if hasAccurateDefaultCheckBeenPerformed(in: window) {
      return false
    }

    return true
  }

  /// Performs a default browser check using the IsDefaultAppChecker and updates the status
  func performAccurateDefaultCheckIfNeeded() {
    if !shouldPerformAccurateDefaultCheck {
      return
    }
    do {
      Preferences.General.isDefaultAPILastCheckDate.value = now()
      let isDefault = try isDefaultAppChecker.isDefaultWebBrowser()
      // Cache the result and current date
      Preferences.General.isDefaultAPILastResult.value = isDefault
      Preferences.General.isDefaultAPILastResultDate.value = now()
    } catch {
      // Handle rate limiting or other errors - keep existing cached values
    }
    updateStatus()
  }

  func recordAppLaunchedWithWebURL() {
    Preferences.General.lastHTTPURLOpenedDate.value = now()
    updateStatus()
  }

  /// Updates the default browser status based on the last accurate check and the last time a user
  /// opened the app using an http url
  func updateStatus() {
    // If API is not available, fall back to old heuristic logic
    guard isDefaultAppChecker.isAvailable() else {
      status = isLastHTTPURLOpenedInPast14Days ? .likely : .unknown
      return
    }

    // Check for cached API result within 14-day validity window
    let fourteenDaysAgo = now().addingTimeInterval(-14.days)
    if let cachedResult = Preferences.General.isDefaultAPILastResult.value,
      let cachedDate = Preferences.General.isDefaultAPILastResultDate.value,
      cachedDate >= fourteenDaysAgo
    {
      // HTTP url opened logic: if user opened a link more recently than the cached check,
      // and that was within the last 14 days, assume they are default
      if let lastHTTPDate = Preferences.General.lastHTTPURLOpenedDate.value,
        lastHTTPDate > cachedDate,
        lastHTTPDate >= fourteenDaysAgo
      {
        status = cachedResult ? .defaulted : .likely
        return
      }

      status = cachedResult ? .defaulted : .notDefaulted
      return
    }

    // No valid cached result - use fallback heuristic
    status = isLastHTTPURLOpenedInPast14Days ? .likely : .unknown
  }

  private var isLastHTTPURLOpenedInPast14Days: Bool {
    guard let date = Preferences.General.lastHTTPURLOpenedDate.value else { return false }
    return date >= now().addingTimeInterval(-14.days)
  }

  private enum DefaultCheckWindow {
    case firstOpen  // Day 0
    case days1to2  // Days 1-2
    case days3to6  // Days 3-6
    case days7to10  // Days 7-10

    init?(daysSinceInstall: Int) {
      switch daysSinceInstall {
      case 0: self = .firstOpen
      case 1...2: self = .days1to2
      case 3...6: self = .days3to6
      case 7...10: self = .days7to10
      default: return nil  // No more checks after day 10
      }
    }
  }

  private func hasAccurateDefaultCheckBeenPerformed(in window: DefaultCheckWindow) -> Bool {
    guard let lastShownDate = Preferences.General.isDefaultAPILastCheckDate.value,
      let installDate = Preferences.DAU.appRetentionLaunchDate.value
    else { return false }

    // Calculate which window the last check was shown in
    let daysBetweenInstallAndLastShown =
      Calendar.current.dateComponents([.day], from: installDate, to: lastShownDate).day ?? 0
    let lastCheckWindow = DefaultCheckWindow(daysSinceInstall: daysBetweenInstallAndLastShown)
    return lastCheckWindow == window
  }
}

struct IsDefaultAppChecker {
  enum CheckError: Error {
    case rateLimited(retryAvailableDate: Date?, statusLastProvidedDate: Date?)
    case unavailable
    case unknown
  }
  /// Whether or not the app checker is available
  var isAvailable: () -> Bool
  /// Whether or not this app is the default web browser, throws a `CheckError` if the system can't
  /// return a result.
  var isDefaultWebBrowser: () throws -> Bool
}

extension IsDefaultAppChecker {
  static var live: Self {
    .init(
      isAvailable: {
        if #available(iOS 18.2, *) {
          return true
        }
        return false
      },
      isDefaultWebBrowser: {
        if #available(iOS 18.2, *) {
          do {
            return try UIApplication.shared.isDefault(.webBrowser)
          } catch let error as UIApplication.CategoryDefaultError {
            switch error.code {
            case .rateLimited:
              throw CheckError.rateLimited(
                retryAvailableDate: error.userInfo[
                  UIApplication.CategoryDefaultError.retryAvailableDateErrorKey
                ] as? Date,
                statusLastProvidedDate: error.userInfo[
                  UIApplication.CategoryDefaultError.statusLastProvidedDateErrorKey
                ] as? Date
              )
            @unknown default:
              throw CheckError.unknown
            }
          } catch {
            throw CheckError.unknown
          }
        }
        throw CheckError.unavailable
      }
    )
  }
}

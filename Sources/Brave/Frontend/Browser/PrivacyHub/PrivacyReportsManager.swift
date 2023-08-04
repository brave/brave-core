// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Preferences
import Data
import UIKit
import BraveVPN
import os.log

public struct PrivacyReportsManager {

  // MARK: - Data processing
  
  /// For performance reasons the blocked requests are not persisted in the database immediately.
  /// Instead a periodic timer is run and all requests gathered during this timeframe are saved in one database transaction.
  public static var pendingBlockedRequests: [(host: String, domain: URL, date: Date)] = []
  
  private static func processBlockedRequests() {
    let itemsToSave = pendingBlockedRequests
    pendingBlockedRequests.removeAll()
    
    // To handle any weird edge cases when user disables data capturing while there are pending items to save
    // we drop them before saving to DB.
    if !Preferences.PrivacyReports.captureShieldsData.value { return }

    BlockedResource.batchInsert(items: itemsToSave)
  }

  private static var saveBlockedResourcesTimer: Timer?
  private static var vpnAlertsTimer: Timer?

  public static func scheduleProcessingBlockedRequests(isPrivateBrowsing: Bool) {
    saveBlockedResourcesTimer?.invalidate()
    
    let timeInterval = AppConstants.buildChannel.isPublic ? 60.0 : 10.0

    saveBlockedResourcesTimer = Timer.scheduledTimer(withTimeInterval: timeInterval, repeats: true) { _ in
      if !isPrivateBrowsing {
        processBlockedRequests()
      }
    }
  }
  
  public static func scheduleVPNAlertsTask() {
    vpnAlertsTimer?.invalidate()
    
    // Because fetching VPN alerts involves making a url request,
    // the time interval to fetch them is longer than the local on-device blocked request processing.
    let timeInterval = AppConstants.buildChannel.isPublic ? 5.minutes : 1.minutes
    vpnAlertsTimer = Timer.scheduledTimer(withTimeInterval: timeInterval, repeats: true) { _ in
      if Preferences.PrivacyReports.captureVPNAlerts.value {
        BraveVPN.processVPNAlerts()
      }
    }
  }
  
  public static func clearAllData() {
    BraveVPNAlert.clearData()
    BlockedResource.clearData()
  }
  
  public static func consolidateData(dayRange range: Int = 30) {
    if Preferences.PrivacyReports.nextConsolidationDate.value == nil {
      Preferences.PrivacyReports.nextConsolidationDate.value = Date().advanced(by: 7.days)
    }
      
    if let consolidationDate = Preferences.PrivacyReports.nextConsolidationDate.value, Date() < consolidationDate {
      return
    }
    
    Preferences.PrivacyReports.nextConsolidationDate.value = Date().advanced(by: 7.days)
    
    BlockedResource.consolidateData(olderThan: range)
    BraveVPNAlert.consolidateData(olderThan: range)
  }

  // MARK: - View
  /// Fetches required data to present the privacy reports view and returns the view.
  static func prepareView(isPrivateBrowsing: Bool) -> PrivacyReportsView {
    let last = BraveVPNAlert.last(3)
    let view = PrivacyReportsView(lastVPNAlerts: last, isPrivateBrowsing: isPrivateBrowsing)
    
    Preferences.PrivacyReports.ntpOnboardingCompleted.value = true

    return view
  }

  // MARK: - Notifications

  public static let notificationID = "privacy-report-weekly-notification"

  public static func scheduleNotification(debugMode: Bool) {
    let notificationCenter = UNUserNotificationCenter.current()

    if debugMode {
      cancelNotification()
    }
    
    if !Preferences.PrivacyReports.captureShieldsData.value {
      cancelNotification()
      return
    }

    notificationCenter.getPendingNotificationRequests { requests in
      if !debugMode && requests.contains(where: { $0.identifier == notificationID }) {
        // Already has one scheduled no need to schedule again.
        return
      }

      let content = UNMutableNotificationContent()
      content.title = Strings.PrivacyHub.notificationTitle
      content.body = Strings.PrivacyHub.notificationMessage

      var dateComponents = DateComponents()
      let calendar = Calendar.current
      dateComponents.calendar = calendar

      // For testing purposes, dev and local builds will launch notification few minutes after it's been enabled.
      if debugMode {
        let now = Date()
        let weekday = calendar.component(.weekday, from: now)
        let hour = calendar.component(.hour, from: now)
        let minute = calendar.component(.minute, from: now)
        dateComponents.weekday = weekday
        dateComponents.hour = hour
        dateComponents.minute = minute + 5
      } else {
        // Every Sunday at 11 AM
        dateComponents.weekday = 1
        dateComponents.hour = 11
      }

      let trigger = UNCalendarNotificationTrigger(dateMatching: dateComponents, repeats: true)
      let request = UNNotificationRequest(identifier: notificationID, content: content, trigger: trigger)
      
      notificationCenter.add(request) { error in
        if let error = error {
          Logger.module.error("Scheduling privacy reports notification error: \(error.localizedDescription)")
        }
      }
    }
  }

  static func cancelNotification() {
    UNUserNotificationCenter.current().removePendingNotificationRequests(withIdentifiers: [notificationID])
  }
}

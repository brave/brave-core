// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UserNotifications

private let log = Logger.rewardsLogger

/// Manages the scheduling and cancellation of the monthly ad grant reminder notification
final public class MonthlyAdsGrantReminder {
    
    @available(*, unavailable)
    init() { }
    
    /// The prefix for all notifications scheduled
    static private let idPrefix = "rewards.notification.monthly-claim"
    
    /// Returns true if the given notification is one scheduled by `MonthlyAdsGrantReminder`
    static public func isMonthlyAdsReminderNotification(_ notification: UNNotification) -> Bool {
        return notification.request.identifier.hasPrefix(idPrefix)
    }
    
    /// Get the identifier for a notification given its month
    static private func identifier(for month: Int) -> String {
        return "\(idPrefix)-\(month)"
    }
    
    /// The calendar we will use for all date ops for ad grant reminders
    static private let calendar = Calendar(identifier: .gregorian)
    
    /// Cancels the current month's notification if one exists
    ///
    /// Trigger this when the user claims an ad grant.
    static public func cancelCurrentMonth() {
        let month = calendar.component(.month, from: Date())
        let id = identifier(for: month)
        let center = UNUserNotificationCenter.current()
        center.removePendingNotificationRequests(withIdentifiers: [id])
        center.removeDeliveredNotifications(withIdentifiers: [id])
        log.debug("Cancelled monthly ad grant reminder for month: \(month)")
    }
    
    /// Retrieve the next month as an integer (1-12). For example, if the current month was december,
    /// this would return `1` for January.
    static private func nextMonth() -> Int? {
        guard let nextMonthsDate = calendar.date(byAdding: .month, value: 1, to: Date()) else {
            assertionFailure("Apocalypse...")
            return nil
        }
        return calendar.component(.month, from: nextMonthsDate)
    }
    
    /// Schedules a notification for the following month if one doesn't already exist for that month
    ///
    /// Trigger this when the user views an ad
    static public func schedule() {
        guard let month = nextMonth() else {
            log.error("Failed to obtain month to schedule notification")
            return
        }
        let center = UNUserNotificationCenter.current()
        center.requestAuthorization(options: [.provisional, .alert, .sound, .badge]) { granted, error in
            if let error = error {
                log.error("Failed to request notifications permissions: \(error)")
                return
            }
            if !granted {
                log.info("Not authorized to schedule a notification")
                return
            }
            
            let id = self.identifier(for: month)
            
            center.getPendingNotificationRequests { requests in
                if requests.contains(where: { $0.identifier == id }) {
                    // Already has one scheduled no need to schedule again
                    return
                }
                
                let content = UNMutableNotificationContent()
                content.title = Strings.MonthlyAdsClaimNotificationTitle
                content.body = Strings.MonthlyAdsClaimNotificationBody
                
                let trigger = UNCalendarNotificationTrigger(
                    dateMatching: .init(calendar: self.calendar, month: month, day: 7, hour: 13, minute: 21),
                    repeats: false
                )
                let request = UNNotificationRequest(
                    identifier: id,
                    content: content,
                    trigger: trigger
                )
                center.add(request) { error in
                    if let error = error {
                        log.error("Failed to add notification request: \(request) with error: \(error)")
                        return
                    }
                    log.debug("Scheduled monthly ad grant reminder: \(request). Trigger date: \(String(describing: trigger.nextTriggerDate()))")
                }
            }
        }
    }
}


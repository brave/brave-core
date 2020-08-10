/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

public let ActivityStreamTopSiteCacheSize: Int32 = 16

private let log = Logger.browserLogger

protocol DataObserver {
    var profile: Profile { get }
    var delegate: DataObserverDelegate? { get set }

    func refreshIfNeeded(forceHighlights highlights: Bool, forceTopSites topSites: Bool)
}

protocol DataObserverDelegate: class {
    func didInvalidateDataSources(refresh forced: Bool, highlightsRefreshed: Bool, topSitesRefreshed: Bool)
    func willInvalidateDataSources(forceHighlights highlights: Bool, forceTopSites topSites: Bool)
}

// Make these delegate methods optional by providing default implementations
extension DataObserverDelegate {
    func didInvalidateDataSources(refresh forced: Bool, highlightsRefreshed: Bool, topSitesRefreshed: Bool) {}
    func willInvalidateDataSources(forceHighlights highlights: Bool, forceTopSites topSites: Bool) {}
}

open class PanelDataObservers {
    var activityStream: DataObserver

    init(profile: Profile) {
        self.activityStream = ActivityStreamDataObserver(profile: profile)
    }
}

class ActivityStreamDataObserver: DataObserver {
    let profile: Profile
    let invalidationTime: UInt64
    weak var delegate: DataObserverDelegate?

    fileprivate let events: [Notification.Name] = [.profileDidFinishSyncing, .privateDataClearedHistory]

    init(profile: Profile) {
        self.profile = profile
        self.invalidationTime = OneMinuteInMilliseconds * 15
        events.forEach { NotificationCenter.default.addObserver(self, selector: #selector(self.notificationReceived), name: $0, object: nil) }
    }

    /*
     refreshIfNeeded will refresh the underlying caches for both TopSites and Highlights.
     By default this will only refresh the highlights if the last fetch is older than 15 mins
     By default this will only refresh topSites if KeyTopSitesCacheIsValid is false
     */
    func refreshIfNeeded(forceHighlights highlights: Bool, forceTopSites topSites: Bool) {
        return
    }

    @objc func notificationReceived(_ notification: Notification) {
        switch notification.name {
        case .profileDidFinishSyncing, .privateDataClearedHistory:
             refreshIfNeeded(forceHighlights: true, forceTopSites: true)
        default:
            log.warning("Received unexpected notification \(notification.name)")
        }
    }
}

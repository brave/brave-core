/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

@testable import Client
import Foundation
import Shared
import Storage
import XCTest

open class MockTabQueue: TabQueue {
    open func addToQueue(_ tab: ShareItem) -> Success {
        return succeed()
    }

    open func getQueuedTabs() -> Deferred<Maybe<Cursor<ShareItem>>> {
        return deferMaybe(ArrayCursor<ShareItem>(data: []))
    }

    open func clearQueuedTabs() -> Success {
        return succeed()
    }
}

open class MockPanelDataObservers: PanelDataObservers {
    override init(profile: Profile) {
        super.init(profile: profile)
        self.activityStream = MockActivityStreamDataObserver(profile: profile)
    }
}

open class MockActivityStreamDataObserver: DataObserver {
    public var profile: Profile
    public weak var delegate: DataObserverDelegate?

    init(profile: Profile) {
        self.profile = profile
    }

    public func refreshIfNeeded(forceHighlights highlights: Bool, forceTopSites topsites: Bool) {

    }
}

open class MockProfile: Profile {
    
    // Read/Writeable properties for mocking
    public var files: FileAccessor
    public var logins: BrowserLogins & SyncableLogins & ResettableSyncStorage

    public lazy var panelDataObservers: PanelDataObservers = {
        return MockPanelDataObservers(profile: self)
    }()

    fileprivate let name: String = "mockaccount"

    init() {
        files = MockFiles()
        logins = MockLogins(files: files)
    }

    public func localName() -> String {
        return name
    }

    public func reopen() {
    }

    public func shutdown() {
    }

    public var isShutdown: Bool = false

    lazy public var isChinaEdition: Bool = {
        return Locale.current.identifier == "zh_CN"
    }()

    lazy public var certStore: CertStore = {
        return CertStore()
    }()

    lazy public var searchEngines: SearchEngines = {
        return SearchEngines(files: self.files)
    }()

    lazy public var prefs: Prefs = {
        return MockProfilePrefs()
    }()

    lazy public var recentlyClosedTabs: ClosedTabsStore = {
        return ClosedTabsStore(prefs: self.prefs)
    }()
}

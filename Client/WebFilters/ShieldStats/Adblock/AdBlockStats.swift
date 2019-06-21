/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Deferred

private let log = Logger.browserLogger

fileprivate struct AdblockStatsResource: Hashable {
    let abpWrapper: ABPFilterLibWrapper
    let id: String
    
    init(abpWrapper: ABPFilterLibWrapper = ABPFilterLibWrapper(), id: String) {
        self.abpWrapper = abpWrapper
        self.id = id
    }
    
    static func == (lhs: AdblockStatsResource, rhs: AdblockStatsResource) -> Bool {
        return lhs.id == rhs.id
    }
    
    func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
}

class AdBlockStats: LocalAdblockResourceProtocol {
    static let shared = AdBlockStats()
    
    typealias LocaleCode = String
    static let defaultLocale = "en"
    
    private let blockListFileName = "ABPFilterParserData"
    
    fileprivate var fifoCacheOfUrlsChecked = FifoDict()
    
    fileprivate lazy var adblockStatsResources: [String: ABPFilterLibWrapper] = {
        let generalAdblocker = [AdblockerType.general.identifier: ABPFilterLibWrapper()]
        return generalAdblocker
    }()
    
    let currentLocaleCode: LocaleCode
    
    fileprivate var isRegionalAdblockEnabled: Bool { return Preferences.Shields.useRegionAdBlock.value }
    
    fileprivate init() {
        currentLocaleCode = Locale.current.languageCode ?? AdBlockStats.defaultLocale
        updateRegionalAdblockEnabledState()
    }
    
    func startLoading() {
        loadLocalData(name: blockListFileName, type: "dat") { data in
            self.setDataFile(data: data, id: AdblockerType.general.identifier)
        }
        
        loadDatFilesFromDocumentsDirectory()
    }
    
    private func loadDatFilesFromDocumentsDirectory() {
        let fm = FileManager.default

        guard let folderUrl = fm.getOrCreateFolder(name: AdblockResourceDownloader.folderName) else {
            log.error("Could not get directory with .dat files")
            return
        }
        
        let enumerator = fm.enumerator(at: folderUrl, includingPropertiesForKeys: nil)
        let filePaths = enumerator?.allObjects as? [URL]
        let datFileUrls = filePaths?.filter { $0.pathExtension == "dat" }

        datFileUrls?.forEach {
            let fileName = $0.deletingPathExtension().lastPathComponent
            
            guard let data = fm.contents(atPath: $0.path) else { return }
            setDataFile(data: data, id: fileName)
        }
    }
    
    fileprivate func updateRegionalAdblockEnabledState() {
        if currentLocaleCode == AdBlockStats.defaultLocale { return }
        
        adblockStatsResources[currentLocaleCode] = ABPFilterLibWrapper()
    }
    
    func shouldBlock(_ request: URLRequest, currentTabUrl: URL?) -> Bool {
        guard let url = request.url else {
            return false
        }
        
        // Do not block main frame urls
        // e.g. user clicked on an ad intentionally (adblock could block redirect to requested site)
        if url == currentTabUrl { return false }
        
        let mainDocDomain = stripLocalhostWebServer(request.mainDocumentURL?.host ?? "")
        
        // A cache entry is like: fifoOfCachedUrlChunks[0]["www.microsoft.com_http://some.url"] = true/false for blocking
        let key = "\(mainDocDomain)_" + stripLocalhostWebServer(url.absoluteString)
        
        if let checkedItem = fifoCacheOfUrlsChecked.getItem(key) {
            if checkedItem === NSNull() {
                return false
            } else {
                if let checkedItem = checkedItem as? Bool {
                    return checkedItem
                } else {
                    log.error("Can't cast checkedItem to Bool")
                    return false
                }
            }
        }
        
        var isBlocked = false
        let header = "*/*"
        
        for (id, adblocker) in adblockStatsResources where adblocker.hasDataFile() {
            if id != AdblockerType.general.identifier && !isRegionalAdblockEnabled { continue }
            
            isBlocked = adblocker.isBlockedConsideringType(url.absoluteString,
                                                                      mainDocumentUrl: mainDocDomain,
                                                                      acceptHTTPHeader: header)
            
            if isBlocked { break }
        }

        fifoCacheOfUrlsChecked.addItem(key, value: isBlocked as AnyObject)
        
        return isBlocked
    }
    
    // Firefox has uses urls of the form
    // http://localhost:6571/errors/error.html?url=http%3A//news.google.ca/
    // to populate the browser history, and load+redirect using GCDWebServer
    func stripLocalhostWebServer(_ url: String?) -> String {
        guard let url = url else { return "" }
    
        // I think the ones prefixed with the following are the only ones of concern. There is also about/sessionrestore urls, not sure if we need to look at those
        let token = "?url="
        
        if let range = url.range(of: token) {
            return url[range.upperBound..<url.endIndex].removingPercentEncoding ?? ""
        } else {
            return url
        }
    }
    
    @discardableResult func setDataFile(data: Data, id: String) -> Deferred<()> {
        let completion = Deferred<()>()

        guard let adblocker = adblockStatsResources[id] else { return completion }
        
        adblocker.setDataFile(data)
        
        if adblocker.hasDataFile() {
            completion.fill(())
        }
        return completion
    }
}

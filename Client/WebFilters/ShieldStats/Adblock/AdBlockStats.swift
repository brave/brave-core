/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared

private let log = Logger.browserLogger

class AdBlockStats: LocalAdblockResourceProtocol {
    static let shared = AdBlockStats()
    
    /// File name of bundled general blocklist.
    private let bundledGeneralBlocklist = "ABPFilterParserData"
    
    fileprivate var fifoCacheOfUrlsChecked = FifoDict()
    
    // Adblock engine for general adblock lists.
    private let generalAdblockEngine: AdblockRustEngine
    
    /// Adblock engine for regional, non-english locales.
    private var regionalAdblockEngine: AdblockRustEngine?
    
    fileprivate var isRegionalAdblockEnabled: Bool { return Preferences.Shields.useRegionAdBlock.value }
    
    fileprivate init() {
        generalAdblockEngine = AdblockRustEngine()
    }
    
    static let adblockSerialQueue = DispatchQueue(label: "com.brave.adblock-dispatch-queue")
    
    func startLoading() {
        parseBundledGeneralBlocklist()
        loadDownloadedDatFiles()
    }
    
    private func parseBundledGeneralBlocklist() {
        guard let path = Bundle.main.path(forResource: bundledGeneralBlocklist, ofType: "dat") else {
            log.error("Can't find path for bundled general blocklist")
            return
        }
        let fileUrl = URL(fileURLWithPath: path)
        
        do {
            let data = try Data(contentsOf: fileUrl)
            AdBlockStats.adblockSerialQueue.async {
                self.generalAdblockEngine.set(data: data)
            }
        } catch {
            log.error("Failed to parse bundled general blocklist: \(error)")
        }
    }
    
    private func loadDownloadedDatFiles() {
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
    
    func shouldBlock(_ request: URLRequest, currentTabUrl: URL?) -> Bool {
        guard let url = request.url, let requestHost = url.host else {
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
        
        isBlocked = generalAdblockEngine.shouldBlock(requestUrl: url.absoluteString, requestHost: requestHost, sourceHost: mainDocDomain)
        
        // Main adblocker didn't catch this rule, checking regional filters if applicable.
        if !isBlocked, isRegionalAdblockEnabled, let regionalAdblocker = regionalAdblockEngine {
            isBlocked = regionalAdblocker.shouldBlock(requestUrl: url.absoluteString, requestHost: requestHost, sourceHost: mainDocDomain)
        }
        
        fifoCacheOfUrlsChecked.addItem(key, value: isBlocked as AnyObject)
        
        return isBlocked
    }
    
    // Firefox has uses urls of the form
    // http://localhost:6571/errors/error.html?url=http%3A//news.google.ca/
    // to populate the browser history, and load+redirect using GCDWebServer
    private func stripLocalhostWebServer(_ url: String?) -> String {
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
        
        if !isGeneralAdblocker(id: id) && regionalAdblockEngine == nil {
            regionalAdblockEngine = AdblockRustEngine()
        }
        
        guard let engine = isGeneralAdblocker(id: id) ? generalAdblockEngine : regionalAdblockEngine else {
            log.error("Adblock engine with id: \(id) is nil")
            return completion
        }
        
        AdBlockStats.adblockSerialQueue.async {
            if engine.set(data: data) {
                log.debug("Adblock file with id: \(id) deserialized successfully")
                // Clearing the cache or checked urls.
                // The new list can bring blocked resource that were previously set as not-blocked.
                self.fifoCacheOfUrlsChecked = FifoDict()
                completion.fill(())
            } else {
                log.error("Failed to deserialize adblock list with id: \(id)")
            }
        }
        
        return completion
    }
    
    private func isGeneralAdblocker(id: String) -> Bool {
        return id == AdblockerType.general.identifier || id == bundledGeneralBlocklist
    }
}

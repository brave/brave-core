// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import BraveRewards

private let logger = Logger.browserLogger

protocol NTPDownloaderDelegate: class {
    func onSponsorUpdated(sponsor: NTPSponsor?)
    func onThemeUpdated(theme: CustomTheme?)
    func preloadCustomTheme(theme: CustomTheme?)
}

class NTPDownloader {
    enum ResourceType {
        /// Downloaded only when users installs the app via special referral code.
        case superReferral(code: String)
        /// Downloaded for all users if a sponsor is available in their region.
        case sponsor
        
        var resourceBaseURL: URL? {
            // This should _probably_ correspond host for URP
            let baseUrl = AppConstants.buildChannel == .debug
                ? "https://mobile-data-dev.s3.brave.software"
                : "https://mobile-data.s3.brave.com/"
            
            switch self {
            case .superReferral(let code):
                return URL(string: baseUrl)?
                    .appendingPathComponent("superreferrer")
                    .appendingPathComponent(code)
            case .sponsor:
                guard let region = Locale.current.regionCode else { return nil }
                return URL(string: baseUrl)?.appendingPathComponent(region)
            }
        }
        
        /// Name of the metadata file on the server.
        var resourceName: String {
            switch self {
            case .superReferral: return "data.json"
            case .sponsor: return "photo.json"
            }
        }
        
        /// Where the file is saved locally.
        var saveLocation: URL? {
            guard let baseUrl = FileManager.default
                .urls(for: .applicationSupportDirectory, in: .userDomainMask).first?
                .appendingPathComponent(self.saveTopFolderName) else { return nil }
            
            switch self {
            case .sponsor:
                return baseUrl
            case .superReferral(let code):
                // Each super referral is saved in separate folder,
                // this allows to have multiple themes installed on the device in the future.
                return baseUrl.appendingPathComponent(code)
            }
        }
        
        /// Resources are downloaded in different folders to support multiple themes and mixing super referrals with branded images
        /// in the future.
        var saveTopFolderName: String {
            switch self {
            case .sponsor: return "NTPDownloads"
            case .superReferral: return "Themes"
            }
        }
    }
    
    /// Returns resource type that should be fetched given present app state.
    var currentResourceType: ResourceType {
        if let currentTheme = Preferences.NewTabPage.selectedCustomTheme.value {
            return .superReferral(code: currentTheme)
        }
        
        if let retryDeadline = Preferences.NewTabPage.superReferrerThemeRetryDeadline.value,
            let refCode = Preferences.URP.referralCode.value {
            
            if Date() < retryDeadline {
                return .superReferral(code: refCode)
            }
        }
        
        return .sponsor
    }
    
    /// Folder where custom favicons are stored.
    static let faviconOverridesDirectory = "favorite_overrides"
    /// For each favicon override, there should be a file that contains info of what background color to use.
    static let faviconOverridesBackgroundSuffix = ".background_color"
    
    private static let etagFile = "crc.etag"
    private var timer: Timer?
    private var backgroundObserver: NSObjectProtocol?
    private var foregroundObserver: NSObjectProtocol?

    weak var delegate: NTPDownloaderDelegate?
    
    deinit {
        self.removeObservers()
    }
    
    func preloadCustomTheme() {
        guard let themeId = Preferences.NewTabPage.selectedCustomTheme.value else { return }
        let customTheme = loadNTPResource(for: .superReferral(code: themeId)) as? CustomTheme
        
        delegate?.preloadCustomTheme(theme: customTheme)
    }
    
    private func getNTPResource(for type: ResourceType, _ completion: @escaping (NTPThemeable?) -> Void) {
        //Load from cache because the time since the last fetch hasn't expired yet..
        if let nextDate = Preferences.NTP.ntpCheckDate.value,
            Date().timeIntervalSince1970 - nextDate < 0 {
            
            if self.timer == nil {
                let relativeTime = abs(Date().timeIntervalSince1970 - nextDate)
                self.scheduleObservers(relativeTime: relativeTime)
            }
            
            return completion(self.loadNTPResource(for: type))
        }
        
        // Download the NTP resource to a temporary directory
        self.downloadMetadata(type: type) { [weak self] url, cacheInfo, error in
            guard let self = self else { return }
            
            //Start the timer no matter what..
            self.startNTPTimer()
            
            if case .campaignEnded = error {
                do {
                    try self.removeCampaign(type: type)
                } catch {
                    logger.error(error)
                }
                return completion(nil)
            }
            
            if let error = error?.underlyingError() {
                logger.error(error)
                return completion(self.loadNTPResource(for: type))
            }
            
            if let cacheInfo = cacheInfo, cacheInfo.statusCode == 304 {
                logger.debug("NTPDownloader Cache is still valid")
                return completion(self.loadNTPResource(for: type))
            }
            
            guard let url = url else {
                logger.error("Invalid NTP Temporary Downloads URL")
                return completion(self.loadNTPResource(for: type))
            }
            
            //Move contents of `url` directory
            //to somewhere more permanent where we'll load the images from..
 
            do {
                guard let saveLocation = type.saveLocation else { throw "Can't find location to save" }
                
                try FileManager.default.createDirectory(at: saveLocation, withIntermediateDirectories: true, attributes: nil)
                
                if FileManager.default.fileExists(atPath: saveLocation.path) {
                    try FileManager.default.removeItem(at: saveLocation)
                }
                
                try FileManager.default.moveItem(at: url, to: saveLocation)
                
                //Store the ETag
                if let cacheInfo = cacheInfo {
                    self.setETag(cacheInfo.etag, type: type)
                }
            } catch {
                logger.error(error)
            }
            
            completion(self.loadNTPResource(for: type))
        }
    }
    
    func notifyObservers(for type: ResourceType) {
        self.getNTPResource(for: type) { [weak self] item in
            
            switch type {
            case .superReferral(let code):
                if item == nil {
                    // Even if referral is nil we stil want to call this code
                    // to trigger side effects of theme update function.
                    self?.delegate?.onThemeUpdated(theme: item as? CustomTheme)
                    return
                }
                
                self?.delegate?.onThemeUpdated(theme: item as? CustomTheme)
                Preferences.NewTabPage.selectedCustomTheme.value = code
                Preferences.NewTabPage.superReferrerThemeRetryDeadline.value = nil
            case .sponsor:
                self?.delegate?.onSponsorUpdated(sponsor: item as? NTPSponsor)
            }
        }
    }
    
    private func startNTPTimer() {
        let relativeTime = { () -> TimeInterval in
            if !AppConstants.buildChannel.isPublic {
                return 3.minutes
            }
            
            let baseTime = 1.hours
            let minVariance = 1.10 // 10% variance
            let maxVariance = 1.14 // 14% variance
            return baseTime * Double.random(in: ClosedRange<Double>(uncheckedBounds: (lower: minVariance, upper: maxVariance)))
        }()
        
        Preferences.NTP.ntpCheckDate.value = Date().timeIntervalSince1970 + relativeTime
        self.scheduleObservers(relativeTime: relativeTime)
    }
    
    private func removeObservers() {
        self.timer?.invalidate()
        
        if let backgroundObserver = self.backgroundObserver {
            NotificationCenter.default.removeObserver(backgroundObserver)
        }
        
        if let foregroundObserver = self.foregroundObserver {
            NotificationCenter.default.removeObserver(foregroundObserver)
        }
    }
    
    private func scheduleObservers(relativeTime: TimeInterval) {
        let resourceType = currentResourceType
        
        self.removeObservers()
        self.timer = Timer.scheduledTimer(withTimeInterval: relativeTime, repeats: true) { [weak self] _ in
            self?.notifyObservers(for: resourceType)
        }
        
        self.backgroundObserver = NotificationCenter.default.addObserver(forName: UIApplication.willResignActiveNotification, object: nil, queue: .main) { [weak self] _ in
            guard let self = self else { return }
            self.timer?.invalidate()
            self.timer = nil
        }
        
        self.foregroundObserver = NotificationCenter.default.addObserver(forName: UIApplication.didBecomeActiveNotification, object: nil, queue: .main) { [weak self] _ in
            guard let self = self else { return }
            
            self.timer?.invalidate()
            self.timer = nil
            
            //If the time hasn't passed yet, reschedule the timer with the relative time..
            if let nextDate = Preferences.NTP.ntpCheckDate.value,
                Date().timeIntervalSince1970 - nextDate < 0 {
                
                let relativeTime = abs(Date().timeIntervalSince1970 - nextDate)
                self.timer = Timer.scheduledTimer(withTimeInterval: relativeTime, repeats: true) { [weak self] _ in
                    self?.notifyObservers(for: resourceType)
                }
            } else {
                //Else the time has already passed so download the new data, reschedule the timers and notify the observers
                self.notifyObservers(for: resourceType)
            }
        }
    }
    
    private func loadNTPResource(for type: ResourceType) -> NTPThemeable? {
        do {
            let metadataFileURL = try self.ntpMetadataFileURL(type: type)
            if !FileManager.default.fileExists(atPath: metadataFileURL.path) {
                return nil
            }
            
            let metadata = try Data(contentsOf: metadataFileURL)
            if self.isCampaignEnded(data: metadata) {
                try self.removeCampaign(type: type)
                return nil
            }
            
            guard let downloadsFolderURL = type.saveLocation else { throw "Can't find location to save" }
            
            switch type {
            case .sponsor:
                let sponsor = try JSONDecoder().decode(NTPSponsor.self, from: metadata)
                                
                let logo = sponsor.logo.map {
                    NTPLogo(
                        imageUrl: downloadsFolderURL.appendingPathComponent($0.imageUrl).path,
                        alt: $0.alt,
                        companyName: $0.companyName,
                        destinationUrl: $0.destinationUrl)
                }
                
                let wallpapers = mapNTPBackgroundsToFullPath(sponsor.wallpapers, basePath: downloadsFolderURL)
                
                return NTPSponsor(wallpapers: wallpapers, logo: logo)
            case .superReferral(let code):
                let customTheme = try JSONDecoder().decode(CustomTheme.self, from: metadata)
                
                let wallpapers = mapNTPBackgroundsToFullPath(customTheme.wallpapers, basePath: downloadsFolderURL)
                
                // At the moment we do not anything with logo for super referrals.
                let logo: NTPLogo? = nil
                
                return CustomTheme(themeName: customTheme.themeName, wallpapers: wallpapers, logo: logo,
                                   topSites: customTheme.topSites, refCode: code)
            }
        } catch {
            logger.error(error)
        }
        
        return nil
    }
    
    private func mapNTPBackgroundsToFullPath(_ backgrounds: [NTPBackground],
                                             basePath: URL) -> [NTPBackground] {
        backgrounds.map {
            NTPBackground(imageUrl: basePath.appendingPathComponent($0.imageUrl).path,
                          focalPoint: $0.focalPoint)
        }
    }
    
    private func getETag(type: ResourceType) -> String? {
        do {
            let etagFileURL = try self.ntpETagFileURL(type: type)
            if !FileManager.default.fileExists(atPath: etagFileURL.path) {
                return nil
            }
            
            return try? String(contentsOfFile: etagFileURL.path, encoding: .utf8)
        } catch {
            logger.error(error)
            return nil
        }
    }
    
    private func setETag(_ etag: String, type: ResourceType) {
        do {
            let etagFileURL = try self.ntpETagFileURL(type: type)
            try etag.write(to: etagFileURL, atomically: true, encoding: .utf8)
        } catch {
            logger.error(error)
        }
    }
    
    private func removeETag(type: ResourceType) throws {
        let etagFileURL = try self.ntpETagFileURL(type: type)
        if FileManager.default.fileExists(atPath: etagFileURL.path) {
            try FileManager.default.removeItem(at: etagFileURL)
        }
    }
    
    func removeCampaign(type: ResourceType) throws {
        try self.removeETag(type: type)
        guard let saveLocation = type.saveLocation else { throw "Can't find location to save" }
        
        switch type {
        case .superReferral(let code):
            Preferences.NewTabPage.selectedCustomTheme.value = nil
            // Force to download assets for regular sponsored resource on next app launch.
            Preferences.NTP.ntpCheckDate.value = nil
            var installedThemes = Preferences.NewTabPage.installedCustomThemes.value
            
            installedThemes.removeAll(where: { $0 == code })
            Preferences.NewTabPage.installedCustomThemes.value = installedThemes
        case .sponsor:
            break
        }
        
        if FileManager.default.fileExists(atPath: saveLocation.path) {
            try FileManager.default.removeItem(at: saveLocation)
        }
    }
    
    private func downloadMetadata(type: ResourceType, _ completion: @escaping (URL?, CacheResponse?, NTPError?) -> Void) {
        self.download(type: type, path: type.resourceName, etag: self.getETag(type: type)) { [weak self] data, cacheInfo, error in
            guard let self = self else { return }
            
            if let error = error {
                return completion(nil, nil, .metadataError(error))
            }
            
            if let cacheInfo = cacheInfo, cacheInfo.statusCode == 304 {
                return completion(nil, cacheInfo, nil)
            }
            
            guard let data = data else {
                return completion(nil, nil, .metadataError("Invalid \(type.resourceName) for NTP Download"))
            }
            
            if self.isCampaignEnded(data: data) {
                return completion(nil, nil, .campaignEnded)
            }
            
            self.unpackMetadata(type: type, data: data) { url, error in
                completion(url, cacheInfo, error)
            }
        }
    }
    
    //MARK: - Download & Unpacking
    
    private func parseETagResponseInfo(_ response: HTTPURLResponse) -> CacheResponse {
        if let etag = response.allHeaderFields["Etag"] as? String {
            return CacheResponse(statusCode: response.statusCode, etag: etag)
        }
        
        if let etag = response.allHeaderFields["ETag"] as? String {
            return CacheResponse(statusCode: response.statusCode, etag: etag)
        }
        
        return CacheResponse(statusCode: response.statusCode, etag: "")
    }
    
    // Downloads the item at the specified url relative to the baseUrl
    private func download(type: ResourceType, path: String?, etag: String?, _ completion: @escaping (Data?, CacheResponse?, Error?) -> Void) {
        guard var url = type.resourceBaseURL else {
            return completion(nil, nil, nil)
        }
        
        if let path = path {
            url = url.appendingPathComponent(path)
        }
        
        var request = URLRequest(url: url)
        if let etag = etag {
            request.setValue(etag, forHTTPHeaderField: "If-None-Match")
        }
        
        URLSession(configuration: .ephemeral).dataRequest(with: request) { [weak self] data, response, error in
            guard let self = self else { return }
            
            if let error = error {
                completion(nil, nil, error)
                return
            }
            
            guard let response = response as? HTTPURLResponse else {
                completion(nil, nil, "Response is not an HTTP Response")
                return
            }
            
            if response.statusCode != 304 && (response.statusCode < 200 || response.statusCode > 299) {
                completion(nil, nil, "Invalid Response Status Code: \(response.statusCode)")
                return
            }
            
            completion(data, self.parseETagResponseInfo(response), nil)
        }
    }
    
    // Unpacks NTPResource by downloading all of its assets to a temporary directory
    // and returning the URL to the directory
    private func unpackMetadata(type: ResourceType, data: Data, _ completion: @escaping (URL?, NTPError?) -> Void) {
        let tempDirectory = FileManager.default.temporaryDirectory
        let directory = tempDirectory.appendingPathComponent(type.saveTopFolderName)
        
        do {
            var error: Error?
            let group = DispatchGroup()
            
            try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true, attributes: nil)
            
            func decodeAndSave(type: ResourceType) throws -> NTPBackgroundProtocol {
                switch type {
                case .sponsor:
                    let item = try JSONDecoder().decode(NTPSponsor.self, from: data)
                    let metadataFileURL = directory.appendingPathComponent(type.resourceName)
                    try JSONEncoder().encode(item).write(to: metadataFileURL, options: .atomic)
                    return item
                case .superReferral:
                    let item = try JSONDecoder().decode(CustomTheme.self, from: data)
                    let metadataFileURL = directory.appendingPathComponent(type.resourceName)
                    try JSONEncoder().encode(item).write(to: metadataFileURL, options: .atomic)
                    return item
                }
            }
            
            let item = try decodeAndSave(type: type)
            
            var imagesToDownload = [String]()
            
            if let logo = item.logo {
                imagesToDownload.append(logo.imageUrl)
            }
            
            imagesToDownload.append(contentsOf: item.wallpapers.map { $0.imageUrl })
            
            for itemURL in imagesToDownload {
                group.enter()
                self.download(type: type, path: itemURL, etag: nil) { data, _, err in
                    if let err = err {
                        error = err
                        group.leave()
                        return
                    }
                    
                    guard let data = data else {
                        error = "No Data Available for NTP-Download: \(itemURL)"
                        group.leave()
                        return
                    }
                    
                    do {
                        let file = directory.appendingPathComponent(itemURL)
                        try data.write(to: file, options: .atomicWrite)
                    } catch let err {
                        error = err
                    }
                    
                    group.leave()
                }
            }
            
            if let customTheme = item as? CustomTheme, let topSites = customTheme.topSites {
                /// For favicons we do not move them to temp directory but write directly to a folder with favicon overrides.
                guard let saveLocation =
                    FileManager.default.getOrCreateFolder(name: NTPDownloader.faviconOverridesDirectory) else {
                        throw "Failed to create directory for favicon overrides"
                }
                
                for topSite in topSites {
                    group.enter()
                    
                    self.download(type: type, path: topSite.iconUrl, etag: nil) { data, _, err in
                        if let err = err {
                            error = err
                            group.leave()
                            return
                        }
                        
                        guard let data = data else {
                            error = "No Data Available for top site: \(topSite.destinationUrl)"
                            group.leave()
                            return
                        }
                        
                        do {
                            guard let baseDomain = URL(string: topSite.destinationUrl)?.baseDomain else {
                                throw "Can't cast to URL.baseDomain: \(topSite.destinationUrl)"
                            }
                               
                            // FIXME: this saves even if error, should move to temp dir, and then move
                            let file = saveLocation.appendingPathComponent(baseDomain)
                            try data.write(to: file, options: .atomicWrite)
                            
                            let topSiteBackgroundColorFileName =
                                baseDomain + NTPDownloader.faviconOverridesBackgroundSuffix
                            let topSiteBackgroundColorURL = saveLocation.appendingPathComponent(topSiteBackgroundColorFileName)
                            
                            try topSite.backgroundColor.write(to: topSiteBackgroundColorURL,
                                                          atomically: true, encoding: .utf8)
                        } catch let err {
                            error = err
                        }
                        
                        group.leave()
                    }
                }
            }
            
            group.notify(queue: .main) {
                if let error = error {
                    return completion(nil, .unpackError(error))
                }
                
                completion(directory, nil)
            }
        } catch {
            completion(nil, .unpackError(error))
        }
    }
    
    private func ntpETagFileURL(type: ResourceType) throws -> URL {
        guard let saveLocation = type.saveLocation else { throw "Can't find location to save" }
        return saveLocation.appendingPathComponent(NTPDownloader.etagFile)
    }
    
    private func ntpMetadataFileURL(type: ResourceType) throws -> URL {
        guard let saveLocation = type.saveLocation else { throw "Can't find location to save" }
        return saveLocation.appendingPathComponent(type.resourceName)
    }
    
    private func isCampaignEnded(data: Data) -> Bool {
        return data.count <= 5 || String(data: data, encoding: .utf8) == "{\n}\n"
    }
    
    private struct CacheResponse {
        let statusCode: Int
        let etag: String
    }
    
    private enum NTPError: Error {
        case campaignEnded
        case metadataError(Error)
        case unpackError(Error)
        case loadingError(Error)
        
        func underlyingError() -> Error? {
            switch self {
            case .campaignEnded:
                return nil
                
            case .metadataError(let error),
                 .unpackError(let error),
                 .loadingError(let error):
                return error
            }
        }
    }
}

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

private let logger = Logger.browserLogger

protocol NTPDownloaderDelegate: class {
    func onNTPUpdated(ntpInfo: NewTabPageBackgroundDataSource.Sponsor?)
}

class NTPDownloader {
    private static let etagFile = "crc.etag"
    private static let metadataFile = "photo.json"
    private static let ntpDownloadsFolder = "NTPDownloads"
    private static let baseURL = "https://brave-ntp-crx-input-dev.s3-us-west-2.amazonaws.com/"
    
    private var timer: Timer?
    private var backgroundObserver: NSObjectProtocol?
    private var foregroundObserver: NSObjectProtocol?

    weak var delegate: NTPDownloaderDelegate? {
        didSet {
            if delegate != nil {
                self.notifyObservers()
            }
        }
    }
    
    deinit {
        self.removeObservers()
    }
    
    private func getNTPInfo(_ completion: @escaping (NewTabPageBackgroundDataSource.Sponsor?) -> Void) {
        //Load from cache because the time since the last fetch hasn't expired yet..
        if let nextDate = Preferences.NTP.ntpCheckDate.value,
            Date().timeIntervalSince1970 - nextDate < 0 {
            
            if self.timer == nil {
                let relativeTime = abs(Date().timeIntervalSince1970 - nextDate)
                self.scheduleObservers(relativeTime: relativeTime)
            }
            
            return completion(self.loadNTPInfo())
        }
        
        // Download the NTP Info to a temporary directory
        self.downloadMetadata { [weak self] url, cacheInfo, error in
            guard let self = self else { return }
            
            if case .campaignEnded = error {
                do {
                    try self.removeCampaign()
                } catch {
                    logger.error(error)
                }
                
                return completion(nil)
            }
            
            //Start the timer no matter what..
            self.startNTPTimer()
            
            if let error = error?.underlyingError() {
                logger.error(error)
                return completion(self.loadNTPInfo())
            }
            
            if let cacheInfo = cacheInfo, cacheInfo.statusCode == 304 {
                logger.debug("NTPDownloader Cache is still valid")
                return completion(self.loadNTPInfo())
            }
            
            guard let url = url else {
                logger.error("Invalid NTP Temporary Downloads URL")
                return completion(self.loadNTPInfo())
            }
            
            //Move contents of `url` directory
            //to somewhere more permanent where we'll load the images from..
 
            do {
                let downloadsFolderURL = try self.ntpDownloadsURL()
                if FileManager.default.fileExists(atPath: downloadsFolderURL.path) {
                    try FileManager.default.removeItem(at: downloadsFolderURL)
                }
                
                try FileManager.default.moveItem(at: url, to: downloadsFolderURL)
                
                //Store the ETag
                if let cacheInfo = cacheInfo {
                    self.setETag(cacheInfo.etag)
                }
            } catch {
                logger.error(error)
            }
            
            completion(self.loadNTPInfo())
        }
    }
    
    private func notifyObservers() {
        self.getNTPInfo { [weak self] item in
            self?.delegate?.onNTPUpdated(ntpInfo: item)
        }
    }
    
    private func startNTPTimer() {
        let baseTime = 5.hours
        let minVariance = 1.10            //10% variance
        let maxVariance = 1.14            //14% variance
        let relativeTime = baseTime * Double.random(in: ClosedRange<Double>(uncheckedBounds: (lower: minVariance, upper: maxVariance)))
        
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
        self.removeObservers()
        self.timer = Timer.scheduledTimer(withTimeInterval: relativeTime, repeats: true) { [weak self] _ in
            self?.notifyObservers()
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
            
            if let nextDate = Preferences.NTP.ntpCheckDate.value {
                let relativeTime = abs(Date().timeIntervalSince1970 - nextDate)
                self.timer = Timer.scheduledTimer(withTimeInterval: relativeTime, repeats: true) { [weak self] _ in
                    self?.notifyObservers()
                }
            } else {
                self.startNTPTimer()
            }
        }
    }
    
    private func loadNTPInfo() -> NewTabPageBackgroundDataSource.Sponsor? {
        do {
            let metadataFileURL = try self.ntpMetadataFileURL()
            if !FileManager.default.fileExists(atPath: metadataFileURL.path) {
                return nil
            }
            
            let metadata = try Data(contentsOf: metadataFileURL)
            if self.isCampaignEnded(data: metadata) {
                try self.removeCampaign()
                return nil
            }
            
            let downloadsFolderURL = try self.ntpDownloadsURL()
            let itemInfo = try JSONDecoder().decode(NewTabPageBackgroundDataSource.Sponsor.self, from: metadata)
            
            let logo = NewTabPageBackgroundDataSource.Sponsor.Logo(
                imageUrl: downloadsFolderURL.appendingPathComponent(itemInfo.logo.imageUrl).path,
                alt: itemInfo.logo.alt,
                companyName: itemInfo.logo.companyName,
                destinationUrl: itemInfo.logo.destinationUrl)
            
            let wallpapers = itemInfo.wallpapers.map {
                NewTabPageBackgroundDataSource.Background(
                    imageUrl: downloadsFolderURL.appendingPathComponent($0.imageUrl).path,
                    focalPoint: $0.focalPoint,
                    credit: nil)
            }
            
            return NewTabPageBackgroundDataSource.Sponsor(wallpapers: wallpapers, logo: logo)
        } catch {
            logger.error(error)
        }
        
        return nil
    }
    
    private func getETag() -> String? {
        do {
            let etagFileURL = try self.ntpETagFileURL()
            if !FileManager.default.fileExists(atPath: etagFileURL.path) {
                return nil
            }
            
            return try? String(contentsOfFile: etagFileURL.path, encoding: .utf8)
        } catch {
            logger.error(error)
            return nil
        }
    }
    
    private func setETag(_ etag: String) {
        do {
            let downloadsFolderURL = try self.ntpDownloadsURL()
            try FileManager.default.createDirectory(at: downloadsFolderURL, withIntermediateDirectories: true, attributes: nil)
            
            let etagFileURL = try self.ntpETagFileURL()
            let etag = etag.replacingOccurrences(of: "\"", with: "")
            try etag.write(to: etagFileURL, atomically: true, encoding: .utf8)
        } catch {
            logger.error(error)
        }
    }
    
    private func removeETag() throws {
        let etagFileURL = try self.ntpETagFileURL()
        if FileManager.default.fileExists(atPath: etagFileURL.path) {
            try FileManager.default.removeItem(at: etagFileURL)
        }
    }
    
    func removeCampaign() throws {
        Preferences.NTP.ntpCheckDate.value = nil
        self.removeObservers()
        
        try self.removeETag()
        let downloadsFolderURL = try self.ntpDownloadsURL()
        if FileManager.default.fileExists(atPath: downloadsFolderURL.path) {
            try FileManager.default.removeItem(at: downloadsFolderURL)
        }
    }
    
    private func downloadMetadata(_ completion: @escaping (URL?, CacheResponse?, NTPError?) -> Void) {
        self.download(path: NTPDownloader.metadataFile, etag: self.getETag()) { [weak self] data, cacheInfo, error in
            guard let self = self else { return }
            
            if let error = error {
                return completion(nil, nil, .metadataError(error))
            }
            
            if let cacheInfo = cacheInfo, cacheInfo.statusCode == 304 {
                return completion(nil, cacheInfo, nil)
            }
            
            guard let data = data else {
                return completion(nil, nil, .metadataError("Invalid \(NTPDownloader.metadataFile) for NTP Download"))
            }
            
            if self.isCampaignEnded(data: data) {
                return completion(nil, nil, .campaignEnded)
            }
            
            do {
                let item = try JSONDecoder().decode(NewTabPageBackgroundDataSource.Sponsor.self, from: data)
                self.unpackMetadata(item: item) { url, error in
                    completion(url, cacheInfo, error)
                }
            } catch {
                completion(nil, nil, .unpackError(error))
            }
        }
    }
    
    private func getBaseURL() -> URL? {
        guard let url = URL(string: NTPDownloader.baseURL), let region = Locale.current.regionCode else {
            return nil
        }
        
        return url.appendingPathComponent(region)
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
    private func download(path: String?, etag: String?, _ completion: @escaping (Data?, CacheResponse?, Error?) -> Void) {
        guard var url = self.getBaseURL() else {
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
                return completion(nil, nil, error)
            }
            
            guard let response = response as? HTTPURLResponse else {
                return completion(nil, nil, "Response is not an HTTP Response")
            }
            
            if response.statusCode != 304 && (response.statusCode < 200 || response.statusCode > 299) {
                completion(nil, nil, "Invalid Response Status Code: \(response.statusCode)")
            }
            
            completion(data, self.parseETagResponseInfo(response), nil)
        }
    }
    
    // Unpacks NTPItemInfo by downloading all of its assets to a temporary directory
    // and returning the URL to the directory
    private func unpackMetadata(item: NewTabPageBackgroundDataSource.Sponsor, _ completion: @escaping (URL?, NTPError?) -> Void) {
        let tempDirectory = FileManager.default.temporaryDirectory
        let directory = tempDirectory.appendingPathComponent(NTPDownloader.ntpDownloadsFolder)
        
        do {
            try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true, attributes: nil)

            let metadataFileURL = directory.appendingPathComponent(NTPDownloader.metadataFile)
            try JSONEncoder().encode(item).write(to: metadataFileURL, options: .atomic)
            
            var error: Error?
            let group = DispatchGroup()
            let urls = [item.logo.imageUrl] + item.wallpapers.map { $0.imageUrl }
            
            for itemURL in urls {
                group.enter()
                self.download(path: itemURL, etag: nil) { data, _, err in
                    if let err = err {
                        error = err
                        return group.leave()
                    }
                    
                    guard let data = data else {
                        error = "No Data Available for NTP-Download: \(itemURL)"
                        return group.leave()
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
    
    private func ntpETagFileURL() throws -> URL {
        return try self.ntpDownloadsURL().appendingPathComponent(NTPDownloader.etagFile)
    }
    
    private func ntpMetadataFileURL() throws -> URL {
        return try self.ntpDownloadsURL().appendingPathComponent(NTPDownloader.metadataFile)
    }
    
    private func ntpDownloadsURL() throws -> URL {
        guard let supportDirectory = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first else {
            throw "NTPDownloader - Cannot find Support Directory"
        }
        
        return supportDirectory.appendingPathComponent(NTPDownloader.ntpDownloadsFolder)
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

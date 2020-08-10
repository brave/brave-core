// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared

private let log = Logger.browserLogger

public extension HTTPCookie {
    
    static var locallySavedFile: String {
        return "CookiesData.json"
    }
    
    private static let directory = FileManager.SearchPathDirectory.applicationSupportDirectory.url
    
    typealias Success = Bool
    class func saveToDisk(_ filename: String = HTTPCookie.locallySavedFile, completion: ((Success) -> Void)? = nil) {
        let cookieStore = WKWebsiteDataStore.default().httpCookieStore
        
        /* For reason unkown the callback to getAllCookies is not called, when the save is done from Settings.
         A possibility is https://bugs.webkit.org/show_bug.cgi?id=188242
         Even with the issue being fixed it still sometimes doesn't work.
         The network process is in suspened maybe?
         
         And for some reason fetch cookie records preemptively guarantees this works.
         Best guess is that fetching records brings network process to active.
         
         Same applies to setting cookies back below.
         */
        WKWebsiteDataStore.default().fetchDataRecords(ofTypes: [WKWebsiteDataTypeCookies]) { _ in}
        cookieStore.getAllCookies { cookies in
            guard let baseDir = directory else {
                completion?(false)
                return
            }
            do {
                let data = try NSKeyedArchiver.archivedData(withRootObject: cookies, requiringSecureCoding: false)
                try data.write(to: baseDir.appendingPathComponent(filename))
                completion?(true)
            } catch {
                log.error("Failed to write cookies to disk with error: \(error)")
                completion?(false)
            }            
        }
    }
    
    class func loadFromDisk(_ filename: String = HTTPCookie.locallySavedFile, completion: ((Success) -> Void)? = nil) {
        guard let baseDir = directory else {
            completion?(false)
            return
        }
        do {
            let data = try Data(contentsOf: baseDir.appendingPathComponent(filename), options: Data.ReadingOptions.alwaysMapped)
            if let cookies = try NSKeyedUnarchiver.unarchiveTopLevelObjectWithData(data) as? [HTTPCookie] {
                HTTPCookie.setCookies(cookies) { success in
                    completion?(success)
                }
                return
            } else {
                log.error("Failed to load cookies from disk with error: Invalid data type")
            }
        } catch {
            log.error("Failed to load cookies from disk with error: \(error)")
        }
        completion?(false)
    }
    
    private class func setCookies(_ cookies: [HTTPCookie], completion: ((Success) -> Void)?) {
        let cookieStore = WKWebsiteDataStore.default().httpCookieStore
        // For the purpose of the line below read the comment in saveCookies (in this same commit)
        WKWebsiteDataStore.default().fetchDataRecords(ofTypes: [WKWebsiteDataTypeCookies]) { _ in}
        let dispatchGroup = DispatchGroup()
        cookies.forEach({
            dispatchGroup.enter()
            cookieStore.setCookie($0, completionHandler: dispatchGroup.leave)
        })
        dispatchGroup.notify(queue: .main) {
            completion?(true)
        }
    }
    
    class func deleteLocalCookieFile(_ filename: String = HTTPCookie.locallySavedFile) {
        guard let baseDir = directory else {
            return
        }
        let url = baseDir.appendingPathComponent(filename)
        do {
            try FileManager.default.removeItem(at: url)
        } catch {
            log.error("Failed to delete local cookie file with error: \(error)")
        }
    }
}

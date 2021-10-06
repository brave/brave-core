/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared

private let log = Logger.browserLogger

class HttpsEverywhereStats: LocalAdblockResourceProtocol {
    static let shared = HttpsEverywhereStats()
    static let dataVersion = "6.0"
    
    static let levelDbFileName = "httpse.leveldb"
    let folderName = "https-everywhere-data"
    
    let httpseDb = HttpsEverywhereObjC()
    let loadDbQueue = DispatchQueue(label: "com.brave.loaddb", qos: .userInteractive)
    
    fileprivate init() { }
    
    func startLoading() {
        loadLocalData(name: HttpsEverywhereStats.levelDbFileName, type: "tgz") { data in
            setData(data: data)
        }
    }
    
    func shouldUpgrade(_ url: URL?, _ completion: @escaping (Bool) -> Void) {
        guard let url = url else {
            log.error("Httpse should block called with empty url")
            completion(false)
            return
        }
        
        return tryRedirectingUrl(url, completion)
    }
    
    func loadDb(dir: String, name: String) {
        let path = dir + "/" + name
        if !FileManager.default.fileExists(atPath: path) {
            log.error("Httpse db file doesn't exist")
            return
        }
        
        httpseDb.load(path)
        assert(httpseDb.isLoaded())
    }
    
    func tryRedirectingUrl(_ url: URL, _ completion: @escaping(Bool) -> Void) {
        loadDbQueue.async {
            if url.scheme?.starts(with: "https") == true {
                completion(false)
                return
            }
            
            if let redirectUrl = self.httpseDb.tryRedirectingUrl(url) {
                let result = redirectUrl.isEmpty ? false : true
                completion(result)
            } else {
                completion(false)
            }
        }
    }
    
    func setData(data: Data) {
        guard let folderUrl = FileManager.default.getOrCreateFolder(name: folderName) else { return }
        unzipAndLoad(folderUrl.path, data: data)
    }
    
    func unzipAndLoad(_ dir: String, data: Data) {
        httpseDb.close()
        succeed().upon() { _ in
            
            let fm = FileManager.default
            if fm.fileExists(atPath: dir + "/" + HttpsEverywhereStats.levelDbFileName) {
                do {
                    try FileManager.default.removeItem(atPath: dir + "/" + HttpsEverywhereStats.levelDbFileName)
                } catch { log.error("failed to remove leveldb file before unzip \(error)") }
            }
            
            self.unzipFile(dir: dir, data: data)
            
            self.loadDbQueue.async {
                self.loadDb(dir: dir, name: HttpsEverywhereStats.levelDbFileName)
            }
        }
    }
    
    private func unzipFile(dir: String, data: Data) {
        let unzip = (data as NSData).gunzipped()
        let fm = FileManager.default
        
        do {
            try fm.createFilesAndDirectories(atPath: dir,
                                             withTarData: unzip,
                                             progress: { _ in
            })
        } catch {
            log.error("unzip file error: \(error)")
        }
    }
}

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

private let log = Logger.browserLogger

private struct AdBlockNetworkResource {
    let resource: CachedNetworkResource
    let fileType: FileType
    let type: AdblockerType
}

class AdblockResourceDownloader {
    static let shared = AdblockResourceDownloader()
    
    private let networkManager: NetworkManager
    private let locale: String
    
    static let folderName = "abp-data"
    
    static let endpoint = "https://adblock-data.s3.brave.com/ios"
    
    init(networkManager: NetworkManager = NetworkManager(), locale: String? = Locale.current.languageCode) {
        if locale == nil {
            log.warning("No locale provided, using default one(\"en\")")
        }
        self.locale = locale ?? "en"
        self.networkManager = networkManager
        
        Preferences.Shields.useRegionAdBlock.observe(from: self)
    }
    
    func startLoading() {
        AdblockResourceDownloader.shared.regionalAdblockResourcesSetup()
        AdblockResourceDownloader.shared.generalAdblockResourcesSetup()
    }
    
    func regionalAdblockResourcesSetup() {
        if !Preferences.Shields.useRegionAdBlock.value {
            log.debug("Regional adblocking disabled, aborting attempt to download regional resources")
            return
        }
        
        downloadResources(type: .regional(locale: locale),
                          queueName: "Regional adblock setup").uponQueue(.main) {
                            log.debug("Regional blocklists download and setup completed.")
                            Preferences.Debug.lastRegionalAdblockUpdate.value = Date()
        }
    }
    
    func generalAdblockResourcesSetup() {
        downloadResources(type: .general,
                          queueName: "General adblock setup").uponQueue(.main) {
                            log.debug("General blocklists download and setup completed.")
                            Preferences.Debug.lastGeneralAdblockUpdate.value = Date()
        }
    }
    
    private func downloadResources(type: AdblockerType, queueName: String) -> Deferred<()> {
        let completion = Deferred<()>()

        let queue = DispatchQueue(label: queueName)
        let nm = networkManager
        let folderName = AdblockResourceDownloader.folderName
        
        // file name of which the file will be saved on disk
        let fileName = type.identifier
        
        let completedDownloads = type.associatedFiles.map { fileType -> Deferred<AdBlockNetworkResource> in
            let fileExtension = fileType.rawValue
            let etagExtension = fileExtension + ".etag"
            
            guard let resourceName = type.resourceName(for: fileType),
                var url = URL(string: AdblockResourceDownloader.endpoint) else {
                return Deferred<AdBlockNetworkResource>()
            }
            
            url.appendPathComponent(resourceName)
            url.appendPathExtension(fileExtension)
            
            let etag = fileFromDocumentsAsString("\(fileName).\(etagExtension)", inFolder: folderName)
            let request = nm.downloadResource(with: url, resourceType: .cached(etag: etag),
                                              checkLastServerSideModification: !AppConstants.buildChannel.isPublic)
                .mapQueue(queue) { resource in
                    AdBlockNetworkResource(resource: resource, fileType: fileType, type: type)
            }
            
            return request
        }
        
        all(completedDownloads).uponQueue(queue) { resources in
            // json to content rules compilation happens first, otherwise it makes no sense to proceed further
            // and overwrite old files that were working before.
            self.compileContentBlocker(resources: resources, queue: queue)
                .uponQueue(queue) { _ in
                    if self.writeFilesTodisk(resources: resources, name: fileName, queue: queue) {
                        self.setUpFiles(resources: resources, compileJsonRules: false, queue: queue)
                            .uponQueue(queue) { completion.fill(()) }
                    }
            }
        }
        
        return completion
    }
    
    private func fileFromDocumentsAsString(_ name: String, inFolder folder: String) -> String? {
        guard let folderUrl = FileManager.default.getOrCreateFolder(name: folder) else {
            log.error("Failed to get folder: \(folder)")
            return nil
        }
        
        let fileUrl = folderUrl.appendingPathComponent(name)
        
        guard let data = FileManager.default.contents(atPath: fileUrl.path) else { return nil }
        
        return String(data: data, encoding: .utf8)
    }
    
    private func compileContentBlocker(resources: [AdBlockNetworkResource],
                                       queue: DispatchQueue) -> Deferred<()> {
        let completion = Deferred<()>()
        
        let compiledLists = resources.filter { $0.fileType == .json }
            .map { res -> Deferred<()> in
                guard let blockList = res.type.blockListName else { return Deferred<()>() }
                return blockList.compile(data: res.resource.data)
        }
        
        all(compiledLists).uponQueue(queue) { _ in
            completion.fill(())
        }
        
        return completion
    }
    
    private func writeFilesTodisk(resources: [AdBlockNetworkResource], name: String,
                                  queue: DispatchQueue) -> Bool {
        var fileSaveCompletions = [Bool]()
        let fm = FileManager.default
        let folderName = AdblockResourceDownloader.folderName
        
        resources.forEach {
            let fileName = name + ".\($0.fileType.rawValue)"
            fileSaveCompletions.append(fm.writeToDiskInFolder($0.resource.data, fileName: fileName,
                                                              folderName: folderName))
            
            if let etag = $0.resource.etag, let data = etag.data(using: .utf8) {
                let etagFileName = fileName + ".etag"
                fileSaveCompletions.append(fm.writeToDiskInFolder(data, fileName: etagFileName,
                                                                  folderName: folderName))
            }
            
            if let lastModified = $0.resource.lastModifiedTimestamp,
                let data = String(lastModified).data(using: .utf8) {
                let lastModifiedFileName = fileName + ".lastmodified"
                fileSaveCompletions.append(fm.writeToDiskInFolder(data, fileName: lastModifiedFileName,
                        folderName: folderName))
            }
            
        }
        
        // Returning true if all file saves completed succesfully
        return !fileSaveCompletions.contains(false)
    }
    
    private func setUpFiles(resources: [AdBlockNetworkResource], compileJsonRules: Bool, queue: DispatchQueue) -> Deferred<()> {
        let completion = Deferred<()>()
        var resourceSetup = [Deferred<()>]()
        
        resources.forEach {
            switch $0.fileType {
            case .dat:
                resourceSetup.append(AdBlockStats.shared.setDataFile(data: $0.resource.data,
                                                                     id: $0.type.identifier))
            case .json:
                if compileJsonRules {
                    resourceSetup.append(compileContentBlocker(resources: resources, queue: queue))
                }
            case .tgz:
                break // TODO: Add downloadable httpse list
            }
        }
        all(resourceSetup).uponQueue(queue) { _ in completion.fill(()) }
        return completion
    }
}

extension AdblockResourceDownloader: PreferencesObserver {
    func preferencesDidChange(for key: String) {
        let regionalAdblockPref = Preferences.Shields.useRegionAdBlock
        if key == regionalAdblockPref.key {
            regionalAdblockResourcesSetup()
        }
    }
}

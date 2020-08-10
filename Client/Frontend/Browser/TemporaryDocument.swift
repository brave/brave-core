/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

private let temporaryDocumentOperationQueue = OperationQueue()

class TemporaryDocument: NSObject {
    private weak var tab: Tab?
    private let request: URLRequest
    private let filename: String

    private var localFileURL: URL?
    private var pendingResult: Deferred<URL>?

    init(preflightResponse: URLResponse, request: URLRequest, tab: Tab) {
        self.request = request
        self.filename = preflightResponse.suggestedFilename ?? "unknown"
        self.tab = tab

        super.init()
    }

    deinit {
        // Delete the temp file.
        if let url = localFileURL {
            try? FileManager.default.removeItem(at: url)
        }
    }

    func getURL() -> Deferred<URL> {
        if let url = localFileURL {
            let result = Deferred<URL>()
            result.fill(url)
            return result
        }

        if let result = pendingResult {
            return result
        }

        let result = Deferred<URL>()
        pendingResult = result
        
        if let tab = self.tab, let url = request.url {
            ResourceDownloadManager.downloadResource(for: tab, url: url)
            
            ensureMainThread {
                UIApplication.shared.isNetworkActivityIndicatorVisible = true
            }
        } else {
            onDocumentDownloaded(document: nil, error: nil)
        }

        return result
    }
    
    func onDocumentDownloaded(document: DownloadedResourceResponse?, error: Error?) {
        ensureMainThread {
            UIApplication.shared.isNetworkActivityIndicatorVisible = false
        }
        
        // Store the blob/data in a local temporary file.
        if let document = document, let data = document.data, !data.isEmpty {
            let tempDirectory = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent("TempDocs")
            let url = tempDirectory.appendingPathComponent(filename)
            
            do {
                try FileManager.default.createDirectory(at: tempDirectory, withIntermediateDirectories: true, attributes: nil)
                try FileManager.default.removeItem(at: url)
                try data.write(to: url, options: [.atomic])
                
                localFileURL = url
                pendingResult?.fill(url)
                pendingResult = nil
                return
            } catch {
                // let the error pass through to the below handler..
            }
        }
        
        // If we encounter an error downloading the temp file, just return with the
        // original remote URL so it can still be shared as a web URL.
        if let url = request.url {
            pendingResult?.fill(url)
        }
        
        pendingResult = nil
    }
}

extension TemporaryDocument: URLSessionTaskDelegate, URLSessionDownloadDelegate {
    func urlSession(_ session: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
        ensureMainThread {
            UIApplication.shared.isNetworkActivityIndicatorVisible = false
        }

        // If we encounter an error downloading the temp file, just return with the
        // original remote URL so it can still be shared as a web URL.
        if error != nil, let remoteURL = request.url {
            pendingResult?.fill(remoteURL)
            pendingResult = nil
        }
    }

    func urlSession(_ session: URLSession, downloadTask: URLSessionDownloadTask, didFinishDownloadingTo location: URL) {
        let tempDirectory = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent("TempDocs")
        let url = tempDirectory.appendingPathComponent(filename)

        try? FileManager.default.createDirectory(at: tempDirectory, withIntermediateDirectories: true, attributes: nil)
        try? FileManager.default.removeItem(at: url)

        do {
            try FileManager.default.moveItem(at: location, to: url)
            localFileURL = url
            pendingResult?.fill(url)
            pendingResult = nil
        } catch {
            // If we encounter an error downloading the temp file, just return with the
            // original remote URL so it can still be shared as a web URL.
            if let remoteURL = request.url {
                pendingResult?.fill(remoteURL)
                pendingResult = nil
            }
        }
    }
}

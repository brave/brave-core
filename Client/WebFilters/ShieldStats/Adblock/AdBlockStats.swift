/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Combine
import BraveCore

private let log = Logger.browserLogger

public class AdBlockStats: LocalAdblockResourceProtocol {
  public static let shared = AdBlockStats()

  /// File name of bundled general blocklist.
  private let bundledGeneralBlocklist = "ABPFilterParserData"

  fileprivate var fifoCacheOfUrlsChecked = FifoDict<Bool>()

  // Adblock engine for general adblock lists.
  private let generalAdblockEngine: AdblockEngine

  /// Adblock engine for regional, non-english locales.
  private var regionalAdblockEngine: AdblockEngine?

  /// The task that downloads all the files. Can be cancelled
  private var downloadTask: AnyCancellable?

  fileprivate var isRegionalAdblockEnabled: Bool { return Preferences.Shields.useRegionAdBlock.value }

  fileprivate init() {
    generalAdblockEngine = AdblockEngine()
  }

  static let adblockSerialQueue = DispatchQueue(label: "com.brave.adblock-dispatch-queue")

  public func startLoading() {
    parseBundledGeneralBlocklist()
    loadDownloadedDatFiles()
  }
  
  /// Checks the general and regional engines to see if the request should be blocked.
  ///
  /// - Note: This method is should not be synced on `AdBlockStatus.adblockSerialQueue` and the result is synced on the main thread.
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType, callback: @escaping (Bool) -> Void) {
    Self.adblockSerialQueue.async { [weak self] in
      let shouldBlock = self?.shouldBlock(requestURL: requestURL, sourceURL: sourceURL, resourceType: resourceType) == false
      
      DispatchQueue.main.async {
        callback(shouldBlock)
      }
    }
  }
  
  func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: AdblockEngine.ResourceType) -> Bool {
    let key = [requestURL.absoluteString, sourceURL.absoluteString, resourceType.rawValue].joined(separator: "_")
    
    if let cachedResult = fifoCacheOfUrlsChecked.getElement(key) {
        return cachedResult
    }
    
    let shouldBlock = generalAdblockEngine.shouldBlock(
      requestURL: requestURL,
      sourceURL: sourceURL,
      resourceType: resourceType
    ) || (isRegionalAdblockEnabled && regionalAdblockEngine?.shouldBlock(
      requestURL: requestURL,
      sourceURL: sourceURL,
      resourceType: resourceType
    ) ?? false)
    
    fifoCacheOfUrlsChecked.addElement(shouldBlock, forKey: key)
    return shouldBlock
  }

  private func parseBundledGeneralBlocklist() {
    guard let path = Bundle.current.path(forResource: bundledGeneralBlocklist, ofType: "dat") else {
      log.error("Can't find path for bundled general blocklist")
      return
    }
    let fileUrl = URL(fileURLWithPath: path)

    do {
      let data = try Data(contentsOf: fileUrl)
      AdBlockStats.adblockSerialQueue.async {
        self.generalAdblockEngine.deserialize(data: data)
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

    downloadTask = nil
    let setupFiles = datFileUrls?.compactMap({ url -> AnyPublisher<Void, Error>? in
      let fileName = url.deletingPathExtension().lastPathComponent
      guard let data = fm.contents(atPath: url.path) else { return nil }
      return self.setDataFile(data: data, id: fileName)
    }) ?? []
    
    if !setupFiles.isEmpty {
      downloadTask = Publishers.MergeMany(setupFiles)
        .collect()
        .subscribe(on: DispatchQueue.global(qos: .userInitiated))
        .map({ _ in () })
        .sink { res in
          if case .failure(let error) = res {
            log.error("Failed to Setup Adblock Stats: \(error)")
          }
        } receiveValue: { _ in
          log.debug("Successfully Setup Adblock Stats")
        }
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
    let key = [mainDocDomain, stripLocalhostWebServer(url.absoluteString)].joined(separator: "_")

    if let checkedItem = fifoCacheOfUrlsChecked.getElement(key) {
        return checkedItem
    }

    let isBlocked = generalAdblockEngine.shouldBlock(
      requestUrl: url.absoluteString,
      requestHost: requestHost,
      sourceHost: mainDocDomain
    ) || (isRegionalAdblockEnabled && regionalAdblockEngine?.shouldBlock(
      requestUrl: url.absoluteString,
      requestHost: requestHost,
      sourceHost: mainDocDomain
    ) ?? false)

    fifoCacheOfUrlsChecked.addElement(isBlocked, forKey: key)
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

  func setDataFile(data: Data, id: String) -> AnyPublisher<Void, Error> {
    if !isGeneralAdblocker(id: id) && regionalAdblockEngine == nil {
      regionalAdblockEngine = AdblockEngine()
    }

    guard let engine = isGeneralAdblocker(id: id) ? generalAdblockEngine : regionalAdblockEngine else {
      return Fail(error: "Adblock engine with id: \(id) is nil").eraseToAnyPublisher()
    }

    return Combine.Deferred {
      Future { completion in
        AdBlockStats.adblockSerialQueue.async {
          if engine.deserialize(data: data) {
            log.debug("Adblock file with id: \(id) deserialized successfully")
            // Clearing the cache or checked urls.
            // The new list can bring blocked resource that were previously set as not-blocked.
            self.fifoCacheOfUrlsChecked = FifoDict<Bool>()
            completion(.success(()))
          } else {
            completion(.failure("Failed to deserialize adblock list with id: \(id)"))
          }
        }
      }
    }.eraseToAnyPublisher()
  }

  private func isGeneralAdblocker(id: String) -> Bool {
    return id == AdblockerType.general.identifier || id == bundledGeneralBlocklist
  }
}

extension AdBlockStats {
  func cosmeticFiltersScript(for url: URL) throws -> String? {
    guard let rules = CosmeticFiltersResourceDownloader.shared.cssRules(for: url).data(using: .utf8) else {
      return nil
    }
    
    let model = try JSONDecoder().decode(CosmeticFilterModel.self, from: rules)
    
    var cssRules = ""
    for rule in model.hideSelectors {
      cssRules += "\(rule){display: none !important}\n"
    }
    
    for (key, value) in model.styleSelectors {
      var subRules = ""
      for subRule in value {
        subRules += subRule + ";"
      }
      
      cssRules += "\(key){" + subRules + " !important}\n"
    }

    var injectedScript = model.injectedScript

    if !injectedScript.isEmpty, Preferences.Shields.autoRedirectAMPPages.value {
      injectedScript = [
        /// This boolean is used by a script injected by cosmetic filters and enables that script via this boolean
        /// The script is found here: https://github.com/brave/adblock-resources/blob/master/resources/de-amp.js
        /// - Note: This script is only a smaller part (1 of 3) of de-amping:
        /// The second part is handled by an inected script that redirects amp pages to their canonical links
        /// The third part is handled by debouncing amp links and handled by debouncing rules
        "const deAmpEnabled = true;",
        injectedScript
      ].joined(separator: "\n")
    }
    
    return """
    (function() {
      var head = document.head || document.getElementsByTagName('head')[0];
      if (head == null) {
          return;
      }
      
      var style = document.createElement('style');
      style.type = 'text/css';
    
      var styles = atob("\(cssRules.toBase64())");
      
      if (style.styleSheet) {
        style.styleSheet.cssText = styles;
      } else {
        style.appendChild(document.createTextNode(styles));
      }

      head.appendChild(style);
      
      (function(){
        \(injectedScript)
      })();
    })();
    """
  }
}

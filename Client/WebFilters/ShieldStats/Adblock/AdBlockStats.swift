/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Combine
import BraveCore

private let log = Logger.browserLogger

class AdBlockStats: LocalAdblockResourceProtocol {
  static let shared = AdBlockStats()

  /// File name of bundled general blocklist.
  private let bundledGeneralBlocklist = "ABPFilterParserData"

  fileprivate var fifoCacheOfUrlsChecked = FifoDict()

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
            self.fifoCacheOfUrlsChecked = FifoDict()
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
        \(model.injectedScript)
      })();
    })();
    """
  }
}

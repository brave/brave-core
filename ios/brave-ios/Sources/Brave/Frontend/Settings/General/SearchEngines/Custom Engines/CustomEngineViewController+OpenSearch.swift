// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Favicon
import Fuzi
import UIKit
import os.log

// MARK: Auto Add Engine

extension CustomEngineViewController {

  func addOpenSearchEngine() {
    guard var referenceURLString = openSearchReference?.reference,
      let title = openSearchReference?.title,
      var referenceURL = URL(string: referenceURLString),
      let faviconImage = faviconImage,
      let hostURLString = host?.absoluteString
    else {
      let alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()
      present(alert, animated: true, completion: nil)
      return
    }

    while referenceURLString.hasPrefix("/") {
      referenceURLString.remove(at: referenceURLString.startIndex)
    }

    let constructedReferenceURLString = "\(hostURLString)/\(referenceURLString)"

    if referenceURL.host == nil,
      let constructedReferenceURL = URL(string: constructedReferenceURLString)
    {
      referenceURL = constructedReferenceURL
    }

    downloadOpenSearchXML(
      referenceURL,
      referenceURL: referenceURLString,
      title: title,
      iconImage: faviconImage
    )
  }

  func downloadOpenSearchXML(_ url: URL, referenceURL: String, title: String, iconImage: UIImage) {
    changeAddEditButton(for: .loading)
    view.endEditing(true)

    NetworkManager().downloadResource(with: url) { [weak self] response in
      guard let self = self else { return }

      switch response {
      case .success(let response):
        Task { @MainActor in
          if let openSearchEngine = await OpenSearchParser(pluginMode: true).parse(
            response.data,
            referenceURL: referenceURL,
            image: iconImage,
            isCustomEngine: true
          ) {
            self.addSearchEngine(openSearchEngine)
          } else {
            let alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()

            self.present(alert, animated: true) {
              self.changeAddEditButton(for: .disabled)
            }
          }
        }
      case .failure(let error):
        Logger.module.error("\(error.localizedDescription)")

        let alert = ThirdPartySearchAlerts.failedToAddThirdPartySearch()
        self.present(alert, animated: true) {
          self.changeAddEditButton(for: .disabled)
        }
      }
    }
  }

  func addSearchEngine(_ engine: OpenSearchEngine) {
    let alert = ThirdPartySearchAlerts.addThirdPartySearchEngine(engine) {
      [weak self] alertAction in
      guard let self = self else { return }

      if alertAction.style == .cancel {
        self.changeAddEditButton(for: .enabled)
        return
      }

      Task { @MainActor in
        do {
          try await self.profile.searchEngines.addSearchEngine(engine)
          self.cancel()
        } catch {
          self.handleError(error: SearchEngineError.failedToSave)

          self.changeAddEditButton(for: .disabled)
        }
      }
    }

    self.present(alert, animated: true, completion: {})
  }
}

// MARK: Auto Add Meta Data

extension CustomEngineViewController {

  func checkSupportAutoAddSearchEngine() {
    guard let openSearchEngine = openSearchReference else {
      changeAddEditButton(for: .disabled)
      checkManualAddExists()
      faviconImage = nil

      return
    }

    let searchEngineExists = profile.searchEngines.orderedEngines.contains(where: {
      let nameExists = $0.shortName.lowercased() == openSearchEngine.title?.lowercased() ?? ""

      if let referenceURL = $0.referenceURL {
        return openSearchEngine.reference.contains(referenceURL) || nameExists
      }

      return nameExists
    })

    if searchEngineExists {
      changeAddEditButton(for: .disabled)
      checkManualAddExists()
    } else {
      changeAddEditButton(for: .enabled)
      isAutoAddEnabled = true
    }
  }

  func fetchSearchEngineSupportForHost(_ host: URL) {
    // Do not perform (Open Search) search engine support for engine edit
    guard customEngineActionType == .add else {
      return
    }

    changeAddEditButton(for: .disabled)

    dataTask = URLSession.shared.dataTask(with: host) { [weak self] data, _, error in
      guard let data = data, error == nil else {
        self?.openSearchReference = nil
        return
      }

      ensureMainThread {
        self?.loadSearchEngineMetaData(from: data, url: host)
      }
    }

    dataTask?.resume()
  }

  private func loadSearchEngineMetaData(from data: Data, url: URL) {
    guard let root = try? HTMLDocument(data: data as Data),
      let searchEngineDetails = fetchOpenSearchReference(document: root)
    else {
      openSearchReference = nil
      return
    }

    faviconTask?.cancel()
    faviconTask = Task { @MainActor in
      do {
        let icon = try await FaviconFetcher.loadIcon(
          url: url,
          persistent: !privateBrowsingManager.isPrivateBrowsing
        )
        self.faviconImage = icon.image ?? Favicon.defaultImage
      } catch {
        self.faviconImage = Favicon.defaultImage
      }

      self.openSearchReference = searchEngineDetails
    }
  }

  private func fetchOpenSearchReference(document: HTMLDocument) -> OpenSearchReference? {
    let documentXpath = "//head//link[contains(@type, 'application/opensearchdescription+xml')]"

    for link in document.xpath(documentXpath) {
      if let referenceLink = link["href"], let title = link["title"] {
        return OpenSearchReference(reference: referenceLink, title: title)
      }
    }

    return nil
  }
}

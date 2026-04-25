// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import BraveUI
import FeedKit
import Foundation
import SwiftUI

enum FindFeedsError: Error, Identifiable {
  /// An error occured while attempting to download the page
  case dataTaskError(Error)
  /// The data was either not received or is in the incorrect format
  case invalidData
  /// The data downloaded did not match a supported feed type
  case parserError(ParserError)
  /// No feeds were found at the given URL
  case noFeedsFound

  var localizedDescription: String {
    switch self {
    case .dataTaskError(let error as URLError) where error.code == .notConnectedToInternet:
      return error.localizedDescription
    case .dataTaskError:
      return Strings.BraveNews.addSourceNetworkFailureMessage
    case .invalidData, .parserError:
      return Strings.BraveNews.addSourceInvalidDataMessage
    case .noFeedsFound:
      return Strings.BraveNews.addSourceNoFeedsFoundMessage
    }
  }

  var id: String {
    localizedDescription
  }
}

struct OPMLImporterViewModifier: ViewModifier {
  @Binding var isPresented: Bool
  var dataSource: FeedDataSource
  @State private var opmlParsedResult: OPMLParsedResult?
  @State private var importError: FindFeedsError?

  func body(content: Content) -> some View {
    content
      .fileImporter(
        isPresented: $isPresented,
        allowedContentTypes: [.init("public.opml")!],
        onCompletion: { result in
          switch result {
          case .success(let url):
            Task {
              await importOPML(from: url)
            }
          case .failure:
            break
          }
        }
      )
      .sheet(item: $opmlParsedResult) { result in
        UIKitController(
          UINavigationController(
            rootViewController:
              BraveNewsAddSourceResultsViewController(
                dataSource: dataSource,
                searchedURL: result.url,
                rssFeedLocations: result.locations,
                sourcesAdded: nil
              )
          )
        )
      }
      .alert(item: $importError) { error in
        Alert(
          title: Text(Strings.BraveNews.addSourceFailureTitle),
          message: Text(error.localizedDescription),
          dismissButton: .default(Text(Strings.OKString))
        )
      }
  }

  private struct OPMLParsedResult: Hashable, Identifiable {
    var url: URL
    var locations: [RSSFeedLocation]
    var id: String {
      url.absoluteString
    }
  }

  private func rssLocationFromOPMLOutline(_ outline: OPML.Outline) -> RSSFeedLocation? {
    guard let url = outline.xmlUrl?.asURL else { return nil }
    return .init(title: outline.text, url: url)
  }

  nonisolated private func importOPML(from url: URL) async {
    guard url.isFileURL,
      let data = await AsyncFileManager.default.contents(atPath: url.path(percentEncoded: false))
    else {
      isPresented = false
      importError = .noFeedsFound
      return
    }
    let opml = OPMLParser.parse(data: data)
    await MainActor.run {
      guard let opml = opml else {
        isPresented = false
        importError = .invalidData
        return
      }
      let locations = opml.outlines.compactMap(self.rssLocationFromOPMLOutline)
      if locations.isEmpty {
        isPresented = false
        importError = .noFeedsFound
        return
      }
      opmlParsedResult = .init(url: url, locations: locations)
    }
  }
}

extension View {
  func opmlImporter(
    isPresented: Binding<Bool>,
    dataSource: FeedDataSource
  ) -> some View {
    modifier(OPMLImporterViewModifier(isPresented: isPresented, dataSource: dataSource))
  }
}

// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import Shared
import Fuzi
import FeedKit

struct SearchResultsView: View {
  @ObservedObject var dataSource: FeedDataSource
  var query: String
  var results: SearchResults
  
  @State private var rssResults: RSSSearchResults?
  @State private var rssSearchError: FindFeedsError? // TODO: Take this out
  @State private var rssSearchTask: URLSessionDataTask?
  
  private var urlQuery: URL? {
    URIFixup.getURL(query)
  }
  
  var body: some View {
    List {
      if !results.channels.isEmpty {
        Section {
          ForEach(results.channels, id: \.self) { channel in
            let shouldShowRegionSubtitle = results.channels.filter {
              $0.name == channel.name
            }.count > 1
            ChannelLabel(
              title: channel.name,
              subtitle: shouldShowRegionSubtitle ? channel.localeDescription : nil,
              isFollowing: dataSource.isFollowingChannelBinding(channel: channel)
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text(Strings.BraveNews.channelsHeaderTitle)
        }
      }
      if !results.sources.isEmpty {
        Section {
          ForEach(results.sources) { source in
            SourceLabel(source: source, isFollowing: dataSource.isFollowingSourceBinding(source: source))
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text(Strings.BraveNews.sourcesHeaderTitle)
        }
      }
      if !results.rssFeeds.isEmpty {
        Section {
          ForEach(results.rssFeeds) { feed in
            RSSFeedLabel(feed: feed, isFollowing: dataSource.isFollowingRSSFeedBinding(feed: feed))
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Text(Strings.BraveNews.userSourcesHeaderTitle)
        }
      }
      if let urlQuery {
        Section {
          Button {
            downloadPageData(for: urlQuery) { result in
              switch result {
              case .success(let locations):
                rssResults = .init(locations: locations)
              case .failure(let error):
                rssSearchError = error
              }
            }
          } label: {
            HStack {
              (Text("\(Strings.BraveNews.getFeedsFromSiteButtonTitle) ").font(.footnote.weight(.medium)) + Text(urlQuery.absoluteString).font(.footnote.weight(.semibold)))
                .multilineTextAlignment(.leading)
              Spacer()
              if rssSearchTask?.state == .running {
                ProgressView()
                  .progressViewStyle(.circular)
              }
            }
          }
          .sheet(item: $rssResults) { results in
            UIKitController(
              UINavigationController(
                rootViewController: BraveNewsAddSourceResultsViewController(dataSource: dataSource, searchedURL: urlQuery, rssFeedLocations: results.locations, sourcesAdded: nil)
              )
            )
          }
          .alert(item: $rssSearchError) { error in
            Alert(
              title: Text(Strings.BraveNews.addSourceFailureTitle),
              message: Text(error.localizedDescription),
              dismissButton: .default(Text(Strings.OKString))
            )
          }
        } header: {
          Text(Strings.BraveNews.availableRSSFeedsHeaderTitle)
        }
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(.braveGroupedBackground))
    .overlay(Group {
      if results.isEmpty, urlQuery == nil {
        Text(Strings.BraveNews.noResultsFound)
          .foregroundColor(Color(.secondaryBraveLabel))
      }
    })
  }
  
  private func downloadPageData(for url: URL, _ completion: @escaping (Result<[RSSFeedLocation], FindFeedsError>) -> Void) {
    let session: URLSession = {
      let configuration = URLSessionConfiguration.ephemeral
      configuration.timeoutIntervalForRequest = 5
      return URLSession(configuration: configuration, delegate: nil, delegateQueue: .main)
    }()
    rssSearchTask = session.dataTask(with: url) { (data, response, error) in
      defer { rssSearchTask = nil }
      if let error = error {
        completion(.failure(.dataTaskError(error)))
        return
      }
      guard let data = data,
            let root = try? HTMLDocument(data: data),
            let url = response?.url
      else {
        completion(.failure(.invalidData))
        return
      }
      
      // Check if `data` is actually an RSS feed
      if case .success(let feed) = FeedParser(data: data).parse() {
        // User provided a direct feed
        var title: String?
        switch feed {
        case .atom(let atom):
          title = atom.title
        case .json(let json):
          title = json.title
        case .rss(let rss):
          title = rss.title
        }
        completion(.success([.init(title: title, url: url)]))
        return
      }
      
      // Check if `data` is actually an OPML list
      if let opml = OPMLParser.parse(data: data), !opml.outlines.isEmpty {
        let locations = opml.outlines.compactMap(self.rssLocationFromOPMLOutline)
        completion(locations.isEmpty ? .failure(.noFeedsFound) : .success(locations))
        return
      }
      
      // Ensure page is reloaded to final landing page before looking for
      // favicons
      var reloadUrl: URL?
      for meta in root.xpath("//head/meta") {
        if let refresh = meta["http-equiv"]?.lowercased(), refresh == "refresh",
           let content = meta["content"],
           let index = content.range(of: "URL="),
           let url = NSURL(string: String(content.suffix(from: index.upperBound))) {
          reloadUrl = url as URL
        }
      }
      
      if let url = reloadUrl {
        self.downloadPageData(for: url, completion)
        return
      }
      
      var feeds: [RSSFeedLocation] = []
      let xpath = "//head//link[contains(@type, 'application/rss+xml') or contains(@type, 'application/atom+xml') or contains(@type, 'application/json')]"
      for link in root.xpath(xpath) {
        guard let href = link["href"], let url = URL(string: href, relativeTo: url),
              url.isWebPage(includeDataURIs: false), !InternalURL.isValid(url: url)
        else {
          continue
        }
        feeds.append(.init(title: link["title"], url: url))
      }
      if feeds.isEmpty {
        completion(.failure(.noFeedsFound))
      } else {
        completion(.success(feeds))
      }
    }
    rssSearchTask?.resume()
  }
  
  private func rssLocationFromOPMLOutline(_ outline: OPML.Outline) -> RSSFeedLocation? {
    guard let url = outline.xmlUrl?.asURL else { return nil }
    return .init(title: outline.text, url: url)
  }
  
  private struct RSSSearchResults: Identifiable {
    var locations: [RSSFeedLocation]
    var id: String {
      locations.map(\.id).joined()
    }
  }
}

#if DEBUG
struct SearchResultsView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    Group {
      SearchResultsView(
        dataSource: .init(),
        query: "",
        results: .empty
      )
      .previewDisplayName("No Results")
      SearchResultsView(
        dataSource: .init(),
        query: "",
        results: .init(
          sources: Mock.sources,
          channels: [.init(localeIdentifier: "en_US", name: "Business")],
          rssFeeds: []
        )
      )
      .previewDisplayName("Results")
    }
  }
}
#endif

// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import CodableHelpers
import Foundation
import OrderedCollections
import UIKit

/// Handles fetching Brave News Topics for widgets
struct NewsTopicsModel {
  var fetchNewsTopics: @Sendable (Locale) async -> [NewsTopic]
  var fetchImageThumbnailsForTopics:
    @Sendable ([NewsTopic], CGSize) async -> [NewsTopic.ID: UIImage]
}

extension NewsTopicsModel {
  /// The live implementation of the news topic model
  ///
  /// - note: For now this implementation does not cache the results of the file, since the widget is not
  /// reloaded by the app in any way yet. Hopefully we can move this code to the BraveNews module in the
  /// future without bloating the size of the widget extension and then get all the cacheing mechanisms
  /// from there.
  static var live: Self {
    .init(
      fetchNewsTopics: { currentLocale in
        do {
          let supportedLocales = [
            Locale(identifier: "en_US"),
            Locale(identifier: "en_GB"),
            Locale(identifier: "en_IN"),
          ]
          let targetLocale =
            supportedLocales.first(where: { $0 == currentLocale }) ?? supportedLocales.first(
              where: { $0.region == currentLocale.region }) ?? supportedLocales.first!

          // At the moment there is only english US topics
          let url = URL(
            string:
              "https://brave-today-cdn.brave.com/news-topic-clustering/widget_topic_news.\(targetLocale.identifier).json"
          )!
          let session = URLSession(configuration: .default)
          let (data, _) = try await session.data(for: URLRequest(url: url))

          let topics = Dictionary(
            grouping: try JSONDecoder.topicsDecoder.decode(
              [FailableDecodable<NewsTopic>].self,
              from: data
            )
            .compactMap(\.wrappedValue).sorted(by: >),
            by: \.topicIndex
          )
          let maxCount = topics.values.map(\.count).max() ?? 0
          var articles: OrderedSet<NewsTopic> = .init()
          for i in 0..<maxCount {
            for key in topics.keys.sorted() {
              if let article = topics[key]?[safe: i] {
                articles.append(article)
              }
            }
          }
          return Array(articles)
        } catch {
          return []
        }
      },
      fetchImageThumbnailsForTopics: { topics, thumbnailSize in
        return await withTaskGroup(of: (String, UIImage?).self, returning: [String: UIImage].self) {
          group in
          var images: [String: UIImage] = [:]
          let session = URLSession(
            configuration: .default,
            delegate: nil,
            delegateQueue: {
              let queue = OperationQueue()
              queue.maxConcurrentOperationCount = 3
              return queue
            }()
          )
          for topic in topics {
            guard let imageURL = topic.imageURL else { continue }
            group.addTask {
              do {
                let request = URLRequest(url: imageURL, timeoutInterval: 10)
                let (data, _) = try await session.data(for: request)
                let image = await UIImage(data: data)?.byPreparingThumbnail(ofSize: thumbnailSize)
                return (topic.id, image)
              } catch {
                return (topic.id, nil)
              }
            }
          }
          for await (id, image) in group {
            images[id] = image
          }
          return images
        }
      }
    )
  }
}

extension NewsTopicsModel {
  static var mock: Self {
    let data = try! Data(
      contentsOf: Bundle.main.url(
        forResource: "topics_news.en_US",
        withExtension: "json",
        subdirectory: nil
      )!
    )
    return .init(
      fetchNewsTopics: { _ in
        do {
          let topics = Dictionary(
            grouping: try JSONDecoder.topicsDecoder.decode(
              [FailableDecodable<NewsTopic>].self,
              from: data
            )
            .compactMap(\.wrappedValue).sorted(by: >),
            by: \.topicIndex
          )
          let maxCount = topics.values.map(\.count).max() ?? 0
          var articles: OrderedSet<NewsTopic> = .init()
          for i in 0..<maxCount {
            for key in topics.keys.sorted() {
              if let article = topics[key]?[safe: i] {
                articles.append(article)
              }
            }
          }
          return Array(articles)
        } catch {
          return []
        }
      },
      fetchImageThumbnailsForTopics: { topics, size in
        return topics.reduce(into: [:]) { result, topic in
          result[topic.id] = UIImage()
        }
      }
    )
  }
}

extension JSONDecoder {
  fileprivate static var topicsDecoder: JSONDecoder {
    let decoder = JSONDecoder()
    decoder.dateDecodingStrategy = .secondsSince1970
    return decoder
  }
}

struct NewsTopic: Decodable, Comparable, Identifiable, Hashable {
  var topicIndex: Int
  var title: String
  var description: String?
  var url: URL
  @URLString var imageURL: URL?
  var publisherName: String
  var date: Date
  var score: Double
  var category: String

  enum CodingKeys: String, CodingKey {
    case topicIndex = "topic_index"
    case title
    case description
    case url
    case imageURL = "img"
    case publisherName = "publisher_name"
    case date = "publish_time"
    case score
    case category
  }

  static func < (lhs: Self, rhs: Self) -> Bool {
    return lhs.score < rhs.score
  }

  var id: String {
    url.absoluteString
  }

  func hash(into hasher: inout Hasher) {
    hasher.combine(url)
  }

  static func == (lhs: Self, rhs: Self) -> Bool {
    lhs.url == rhs.url
  }
}

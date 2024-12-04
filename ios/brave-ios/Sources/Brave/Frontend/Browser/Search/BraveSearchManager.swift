// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Combine
import Foundation
import Shared
import UserAgent
import WebKit
import os.log

// A helper class to handle Brave Search fallback needs.
class BraveSearchManager: NSObject {
  private let fallbackProviderURLString = "https://www.google.com/search"

  /// Brave Search query details which are passed to the fallback provider.
  struct BackupQuery: Codable {
    let found: Bool
    let country: String?
    let language: String?
    let safesearch: String?
  }

  /// URL of the Brave Search request we performed.
  /// Contains a query and endpoint(prod or staging)
  private let url: URL
  /// What did we search for using Brave Search
  private let query: String
  /// BraveSearch cookies are used to pass search settings we saved on the website
  /// such as whether to use search fallback, should safe search be performed etc.
  private let domainCookies: [HTTPCookie]

  private let profile: Profile

  /// The result we got from querying the fallback search engine.
  var fallbackQueryResult: String?
  /// Whether the call to the fallback search engine is pending.
  /// This is used to determine at what point of the web navigation we should inject the results.
  var fallbackQueryResultsPending = false {
    didSet {
      // Fallback query ended, time to append debug log with all info we've gathered.
      if let callbackLog = callbackLog, !fallbackQueryResultsPending {
        BraveSearchLogEntry.shared.logs.append(callbackLog)
      }
    }
  }

  private var cancellables: Set<AnyCancellable> = []

  private let authManager = BasicAuthCredentialsManager()

  static let validDomains = [
    "search.brave.com", "search.brave.software",
    "search.bravesoftware.com", "safesearch.brave.com",
    "safesearch.brave.software",
    "safesearch.bravesoftware.com",
    "search-dev-local.brave.com",
  ]

  static func isValidURL(_ url: URL) -> Bool {
    validDomains.contains(url.host ?? "")
  }

  private var callbackLog: BraveSearchLogEntry.FallbackLogEntry?

  init?(profile: Profile, url: URL, cookies: [HTTPCookie]) {
    if !Self.isValidURL(url) {
      return nil
    }

    // Check if the request has a valid query string to check against.
    guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false),
      let queryItem = components.valueForQuery("q")
    else { return nil }

    self.profile = profile
    self.url = url
    self.query = queryItem
    self.domainCookies = cookies.filter { $0.domain == url.host }
    if domainCookies.first(where: { $0.name == "fallback" })?.value != "1" {
      return nil
    }

    if BraveSearchLogEntry.shared.isEnabled {
      callbackLog = .init(date: Date(), url: url, query: queryItem, cookies: domainCookies)
    }
  }

  /// A call is made to the Brave Search api with query we performed.
  /// It returns  details that are used in call to the fallback search engine.
  func shouldUseFallback(completion: @escaping (BackupQuery?) -> Void) {
    guard
      var canAnswerURLComponents =
        URLComponents(string: url.domainURL.absoluteString)
    else {
      completion(nil)
      return
    }

    canAnswerURLComponents.scheme = "https"
    canAnswerURLComponents.path = "/api/can_answer"
    canAnswerURLComponents.queryItems = [.init(name: "q", value: query)]

    guard let url = canAnswerURLComponents.url else {
      completion(nil)
      return
    }

    var request = URLRequest(
      url: url,
      cachePolicy: .reloadIgnoringLocalAndRemoteCacheData,
      timeoutInterval: 5
    )

    let cookieStorage = HTTPCookieStorage()
    domainCookies.forEach { cookieStorage.setCookie($0) }

    let headers = HTTPCookie.requestHeaderFields(with: domainCookies)
    headers.forEach {
      request.setValue($0.value, forHTTPHeaderField: $0.key)
    }

    request.setValue(UserAgent.userAgentForIdiom(), forHTTPHeaderField: "User-Agent")

    let session = URLSession(configuration: .ephemeral, delegate: authManager, delegateQueue: .main)

    var timer: PerformanceTimer?
    if callbackLog != nil {
      timer = PerformanceTimer(label: "Brave Search Debug can answer api call")
    }

    // Important, URLSessionDelegate must have been implemented here
    // to handle request authentication.
    session
      .dataTaskPublisher(for: request)
      .tryMap { [weak self] output -> Data in
        if self?.callbackLog != nil, let timer = timer {
          _ = timer.stop()
          self?.callbackLog?.canAnswerTime = String(format: "%.2f", Double(timer.duration ?? 0))
        }

        guard let response = output.response as? HTTPURLResponse,
          response.statusCode >= 200 && response.statusCode < 300
        else {
          throw URLError(.badServerResponse)
        }

        self?.callbackLog?.backupQuery = String(data: output.data, encoding: .utf8)
        return output.data
      }
      .decode(type: BackupQuery.self, decoder: JSONDecoder())
      .sink(
        receiveCompletion: { status in
          switch status {
          case .failure(let error):
            Logger.module.error("shouldUseFallback error: \(error.localizedDescription)")
            // No subsequent call to backup search engine is made in case of error.
            // The pending status has to be cancelled.
            completion(nil)
          case .finished:
            // Completion is called on `receiveValue`.
            break
          }
        },
        receiveValue: { canAnswer in
          completion(canAnswer)
        }
      )
      .store(in: &cancellables)

    session.finishTasksAndInvalidate()
  }

  /// Perform a backup search using an alternative search engine, gets results back as html source.
  func backupSearch(
    with backupQuery: BackupQuery,
    completion: @escaping (String) -> Void
  ) {

    guard var components = URLComponents(string: fallbackProviderURLString) else { return }

    var queryItems: [URLQueryItem] = [
      .init(name: "q", value: query)
    ]

    if let language = backupQuery.language {
      queryItems.append(.init(name: "hl", value: language))
    }

    if let country = backupQuery.country {
      queryItems.append(.init(name: "gl", value: country))
    }

    components.queryItems = queryItems

    guard let url = components.url else { return }
    var request = URLRequest(
      url: url,
      cachePolicy: .reloadIgnoringLocalCacheData,
      timeoutInterval: 5
    )

    // Must be set, without it the fallback results may be not retrieved correctly.
    request.addValue(UserAgent.userAgentForIdiom(), forHTTPHeaderField: "User-Agent")

    request.addValue(
      "text/html;charset=UTF-8, text/plain;charset=UTF-8",
      forHTTPHeaderField: "Accept"
    )

    var timer: PerformanceTimer?
    if callbackLog != nil {
      timer = PerformanceTimer(label: "Brave Search Debug fallback results call")
    }

    let session = URLSession(configuration: .ephemeral)
    session
      .dataTaskPublisher(for: request)
      .tryMap { [weak self] output -> String in
        if self?.callbackLog != nil, let timer = timer {
          _ = timer.stop()
          self?.callbackLog?.fallbackTime = String(format: "%.2f", Double(timer.duration ?? 0))
        }

        guard let response = output.response as? HTTPURLResponse,
          response.statusCode >= 200 && response.statusCode < 300
        else {
          throw URLError(.badServerResponse)
        }

        guard let stringFromData = String(data: output.data, encoding: .utf8),
          let escapedString = stringFromData.javaScriptEscapedString
        else {
          throw DecodingError.typeMismatch(
            String.self,
            .init(codingPath: [], debugDescription: "Failed to decode string from data")
          )
        }

        self?.callbackLog?.fallbackData = output.data
        return escapedString
      }
      .receive(on: DispatchQueue.main)
      .sink(
        receiveCompletion: { status in
          switch status {
          case .failure(let error):
            Logger.module.error("Error: \(error.localizedDescription)")
          case .finished:
            break
          }
        },
        receiveValue: { [weak self] data in
          self?.fallbackQueryResult = data
          completion(data)
        }
      )
      .store(in: &cancellables)

    session.finishTasksAndInvalidate()
  }
}

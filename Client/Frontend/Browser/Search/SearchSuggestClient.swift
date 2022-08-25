/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

/*
 * Clients of SearchSuggestionClient should retain the object during the
 * lifetime of the search suggestion query, as requests are canceled during destruction.
 *
 * Query callbacks that must run even if they are cancelled should wrap their contents in `withExtendendLifetime`.
 */
class SearchSuggestClient {
  static let errorDomain = "com.brave.ios.SearchSuggestClient"
  static let invalidEngineErrorCode = 0
  static let invalidResponseErrorCode = 1
  
  fileprivate let searchEngine: OpenSearchEngine
  fileprivate var request: URLSessionDataTask?
  fileprivate let userAgent: String

  lazy fileprivate var session: URLSession = {
    let configuration = URLSessionConfiguration.ephemeral
    configuration.httpAdditionalHeaders = ["User-Agent": self.userAgent]
    return URLSession(configuration: configuration, delegate: nil, delegateQueue: .main)
  }()

  init(searchEngine: OpenSearchEngine, userAgent: String) {
    self.searchEngine = searchEngine
    self.userAgent = userAgent
  }

  func query(_ query: String, callback: @escaping (_ response: [String]?, _ error: NSError?) -> Void) {
    let url = searchEngine.suggestURLForQuery(query)
    if url == nil {
      let error = NSError(domain: Self.errorDomain, code: Self.invalidEngineErrorCode, userInfo: nil)
      callback(nil, error)
      return
    }

    request = session.dataTask(
      with: url!,
      completionHandler: { data, response, error in
        if let error = error {
          return callback(nil, error as NSError?)
        }

        let responseError = NSError(domain: Self.errorDomain, code: Self.invalidResponseErrorCode, userInfo: nil)

        if let response = response as? HTTPURLResponse {
          if !(200..<300).contains(response.statusCode) {
            return callback(nil, responseError)
          }
        }

        guard let data = data else {
          return callback(nil, responseError)
        }

        do {
          let result = try JSONSerialization.jsonObject(with: data, options: .mutableLeaves)

          // The response will be of the following format:
          //    ["foobar",["foobar","foobar2000 mac","foobar skins",...]]
          // That is, an array of at least two elements: the search term and an array of suggestions.
          guard let array = result as? NSArray else {
            return callback(nil, responseError)
          }

          if array.count < 2 {
            return callback(nil, responseError)
          }

          let suggestions = array[1] as? [String]
          if suggestions == nil {
            return callback(nil, responseError)
          }

          callback(suggestions!, nil)
        } catch {
          return callback(nil, error as NSError?)
        }
      })
    request?.resume()
  }

  func cancelPendingRequest() {
    request?.cancel()
  }
}

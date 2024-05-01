// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Shared

extension URL {
  /// Obtains a URLOrigin given the current URL
  ///
  /// Creates an Origin from |url|, as described at
  /// https://url.spec.whatwg.org/#origin, with the following additions:
  ///
  /// 1. If |url| is invalid or non-standard, an opaque Origin is constructed.
  /// 2. 'filesystem' URLs behave as 'blob' URLs (that is, the origin is parsed
  ///    out of everything in the URL which follows the scheme).
  /// 3. 'file' URLs all parse as ("file", "", 0).
  ///
  /// Note that the returned Origin may have a different scheme and host from
  /// |url| (e.g. in case of blob URLs - see OriginTest.ConstructFromGURL).
  public var origin: URLOrigin {
    .init(url: self)
  }

  /// Obtains a clean stripped url from the current Internal URL
  ///
  /// Returns the original url without  internal parameters
  public var strippedInternalURL: URL? {
    if let internalURL = InternalURL(self) {
      switch internalURL.urlType {
      case .errorPage:
        return internalURL.originalURLFromErrorPage
      case .web3Page, .sessionRestorePage, .aboutHomePage:
        return internalURL.extractedUrlParam
      case .blockedPage:
        return decodeEmbeddedInternalURL(for: .blocked)
      case .httpBlockedPage:
        return decodeEmbeddedInternalURL(for: .httpBlocked)
      case .readerModePage:
        return decodeEmbeddedInternalURL(for: .readermode)
      default:
        return nil
      }
    }

    return nil
  }

  /// URL returned for display purposes
  public var displayURL: URL? {
    if self.absoluteString.starts(with: "blob:") {
      return self.havingRemovedAuthorisationComponents()
    }

    if self.isFileURL {
      return URL(string: "file://\(self.lastPathComponent)")
    }

    if self.isInternalURL(for: .readermode) {
      return self.decodeEmbeddedInternalURL(for: .readermode)?
        .havingRemovedAuthorisationComponents()
    }

    if let internalUrl = InternalURL(self), internalUrl.isErrorPage {
      return internalUrl.originalURLFromErrorPage?.displayURL
    }

    if let internalUrl = InternalURL(self),
      internalUrl.isSessionRestore || internalUrl.isWeb3URL || internalUrl.isHTTPBlockedPage
        || internalUrl.isBlockedPage
    {
      return internalUrl.extractedUrlParam?.displayURL
    }

    if !InternalURL.isValid(url: self) {
      let url = self.havingRemovedAuthorisationComponents()
      if let internalUrl = InternalURL(url), internalUrl.isErrorPage {
        return internalUrl.originalURLFromErrorPage?.displayURL
      }
      return url
    }

    return nil
  }

  /// Returns true if this is an embedded url for the given `InternalURL.Path`
  public func isInternalURL(for internalPath: InternalURL.Path) -> Bool {
    let scheme = self.scheme
    let host = self.host
    let path = self.path
    return scheme == InternalURL.scheme && host == InternalURL.host
      && path == "/\(internalPath.rawValue)"
  }

  /// Extract an embedded url given by the `url` query param from an interna urll (i.e. `internal://local`)
  public func decodeEmbeddedInternalURL(for internalPath: InternalURL.Path) -> URL? {
    guard self.isInternalURL(for: internalPath) else { return nil }
    let components = URLComponents(url: self, resolvingAgainstBaseURL: false)
    let queryItem = components?.queryItems?.first(where: {
      $0.name == InternalURL.Param.url.rawValue
    })
    guard let value = queryItem?.value else { return nil }
    return URL(string: value)
  }

  /// Embed a url into an internal URL for the given path. The url will be placed in a `url` querey param
  public func encodeEmbeddedInternalURL(
    for path: InternalURL.Path,
    headers: [String: String]? = nil
  ) -> URL? {
    let baseURL = "\(InternalURL.baseUrl)/\(path.rawValue)"

    guard
      let encodedURL = absoluteString.addingPercentEncoding(withAllowedCharacters: .alphanumerics)
    else {
      return nil
    }

    if let headers = headers, !headers.isEmpty,
      let data = try? JSONSerialization.data(withJSONObject: headers),
      let encodedHeaders = data.base64EncodedString.addingPercentEncoding(
        withAllowedCharacters: .alphanumerics
      )
    {
      return URL(
        string:
          "\(baseURL)?\(InternalURL.Param.url.rawValue)=\(encodedURL)&headers=\(encodedHeaders)"
      )
    }

    return URL(string: "\(baseURL)?\(InternalURL.Param.url.rawValue)=\(encodedURL)")
  }
}

extension InternalURL {

  enum URLType {
    case blockedPage
    case httpBlockedPage
    case sessionRestorePage
    case errorPage
    case readerModePage
    case aboutHomePage
    case web3Page
    case other
  }

  var urlType: URLType {
    // This needs to be before `isBlockedPage`
    // because http-blocked has the word "blocked" in it
    // We should refactor this code because its really iffy.
    if isHTTPBlockedPage {
      return .httpBlockedPage
    }

    if isBlockedPage {
      return .blockedPage
    }

    if isErrorPage {
      return .errorPage
    }

    if isWeb3URL {
      return .web3Page
    }

    if isReaderModePage {
      return .readerModePage
    }

    if isSessionRestore {
      return .sessionRestorePage
    }

    if isAboutHomeURL {
      return .aboutHomePage
    }

    return .other
  }
}

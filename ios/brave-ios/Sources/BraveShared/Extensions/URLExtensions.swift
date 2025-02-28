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

    if let internalUrl = InternalURL(self), internalUrl.isBasicAuthURL {
      return self
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

  /// Determines the predominant directionality of the URL's text as defined:
  /// - Left-To-Right - if a string has a strong LTR directionality, it would be laid out LTR.
  /// - Right-To-Left - if a string has a strong RTL directionality, it would be laid out RTL.
  /// - Mixed - if a string contains BOTH a strong LTR and RTL directionality
  /// - Neutral - if a string contains NEITHER a strong LTR or RTL directionality (empty string)
  /// Therefore, a string is `Uni-directional`, if it is LTR or RTL, but not both.
  public var isUnidirectional: Bool {
    // First format the URL which will decode the puny-coding
    var renderedString = URLFormatter.formatURL(
      absoluteString,
      formatTypes: [.omitDefaults, .omitTrivialSubdomains, .omitTrailingSlashOnBareHostname],
      unescapeOptions: .normal
    )

    // Strip scheme
    if let scheme,
      let range = renderedString.range(
        of: "^(\(scheme)://|\(scheme):)",
        options: .regularExpression
      )
    {
      renderedString.replaceSubrange(range, with: "")
    }

    return renderedString.bidiDirection != .mixed
  }

  /// Determines whether the URL would typically be rendered Left-To-Right instead of Right-To-Left
  public var isRenderedLeftToRight: Bool {
    // First format the URL which will decode the puny-coding
    var renderedString = URLFormatter.formatURL(
      absoluteString,
      formatTypes: [.omitDefaults, .omitTrivialSubdomains, .omitTrailingSlashOnBareHostname],
      unescapeOptions: .normal
    )

    // Strip scheme
    if let scheme,
      let range = renderedString.range(
        of: "^(\(scheme)://|\(scheme):)",
        options: .regularExpression
      )
    {
      renderedString.replaceSubrange(range, with: "")
    }

    return renderedString.bidiBaseDirection == .leftToRight
  }

  public var strippingBlobURLAuth: URL {
    if self.scheme == "blob",
      var components = URLComponents(url: self, resolvingAgainstBaseURL: true)
    {
      components.scheme = nil

      if let newURL = components.url {
        if var newComponents = URLComponents(url: newURL, resolvingAgainstBaseURL: true) {
          newComponents.user = nil
          newComponents.password = nil
          newComponents.scheme = newURL.scheme

          if let url = newComponents.url, let result = URL(string: "blob:\(url.absoluteString)") {
            return result
          }
        }
      }
    }
    return self
  }

  /// Matches what `window.origin` would return in javascript.
  public var windowOriginURL: URL {
    var components = URLComponents()
    components.scheme = self.scheme
    components.port = self.port
    components.host = self.host
    return components.url ?? self
  }

  /// Returns true if we should show Shred option for the given URL.
  public var isShredAvailable: Bool {
    urlToShred != nil
  }

  /// The URL to shred. Extracts reader mode url if it's a reader
  /// mode internal url.
  public var urlToShred: URL? {
    if let internalURL = InternalURL(self) {
      if internalURL.isReaderModePage {
        return internalURL.extractedUrlParam
      }
      // only allow reader mode InternalURL to be shred
      return nil
    }
    return self
  }

  // Returns true if a string is a valid URL, with the specified optional scheme,
  // without percent encoding, idn encoding, or modifying the URL in any way.
  public static func isValidURLWithoutEncoding(text: String, scheme: String? = nil) -> Bool {
    guard let components = URLComponents(string: text) else {
      return false
    }

    // If a scheme was specified, check if it matches
    if let scheme = scheme, !scheme.isEmpty, components.scheme != scheme {
      return false
    }

    // A valid URL must have a non-empty host or path
    if (components.host ?? components.path).isEmpty {
      return false
    }

    return true
  }

  public static func uniqueFileName(_ filename: String, in folder: URL) async throws -> URL {
    let basePath = folder.appending(path: filename)
    let fileExtension = basePath.pathExtension
    let filenameWithoutExtension =
      !fileExtension.isEmpty ? String(filename.dropLast(fileExtension.count + 1)) : filename

    var proposedPath = basePath
    var count = 0

    while await AsyncFileManager.default.fileExists(atPath: proposedPath.path) {
      count += 1

      let proposedFilenameWithoutExtension = "\(filenameWithoutExtension) (\(count))"
      proposedPath = folder.appending(path: proposedFilenameWithoutExtension)
        .appending(path: fileExtension)
    }

    return proposedPath
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

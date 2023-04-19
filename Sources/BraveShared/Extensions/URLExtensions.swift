// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
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
  public var stippedInternalURL: URL? {
    if InternalURL.isValid(url: self),
       let internalURL = InternalURL(self) {
      
      switch internalURL.urlType {
      case .errorPage:
        return internalURL.originalURLFromErrorPage
      case .web3Page, .sessionRestorePage, .readerModePage, .aboutHomePage:
        return internalURL.extractedUrlParam
      default:
        return nil
      }
    }
    
    return nil
  }
}

extension InternalURL {
  
  enum URLType {
    case sessionRestorePage
    case errorPage
    case readerModePage
    case aboutHomePage
    case web3Page
    case other
  }
  
  var urlType: URLType {
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

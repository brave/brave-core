// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

class NetworkErrorPageHandler: InterstitialPageHandler {
  static func isNetworkError(errorCode: Int) -> Bool {
    if let code = CFNetworkErrors(rawValue: Int32(errorCode)) {
      return code == .cfurlErrorNotConnectedToInternet
    }

    return errorCode == NSURLErrorNotConnectedToInternet
  }
  
  func canHandle(error: NSError) -> Bool {
    // Handle CFNetwork Error
    if error.domain == kCFErrorDomainCFNetwork as String,
      let code = CFNetworkErrors(rawValue: Int32(error.code)) {

      let handledCodes: [CFNetworkErrors] = [
        .cfurlErrorNotConnectedToInternet
      ]

      return handledCodes.contains(code)
    }

    // Handle NSURLError
    if error.domain == NSURLErrorDomain as String {
      let handledCodes: [Int] = [
        NSURLErrorNotConnectedToInternet
      ]

      return handledCodes.contains(error.code)
    }
    return false
  }

  func response(for model: ErrorPageModel) -> (URLResponse, Data)? {
    guard let asset = Bundle.module.path(forResource: "NetworkError", ofType: "html") else {
      assert(false)
      return nil
    }

    guard var html = try? String(contentsOfFile: asset) else {
      assert(false)
      return nil
    }
    var domain = model.domain

    // Update the error code domain
    if domain == kCFErrorDomainCFNetwork as String,
      let code = CFNetworkErrors(rawValue: Int32(model.errorCode)) {
      domain = GenericErrorPageHandler.CFErrorToName(code)
    } else if domain == NSURLErrorDomain {
      domain = GenericErrorPageHandler.NSURLErrorToName(model.errorCode)
    }

    let host = model.originalURL.normalizedHost(stripWWWSubdomainOnly: true) ?? model.originalHost

    let variables = [
      "page_title": host,
      "error_code": "\(model.errorCode)",
      "error_title": Strings.errorPagesNoInternetTitle,
      "error_domain": domain,
      "error_try_list": Strings.errorPagesNoInternetTry,
      "error_list_1": Strings.errorPagesNoInternetTryItem1,
      "error_list_2": Strings.errorPagesNoInternetTryItem2,
    ]

    variables.forEach { (arg, value) in
      html = html.replacingOccurrences(of: "%\(arg)%", with: value)
    }

    guard let data = html.data(using: .utf8) else {
      return nil
    }

    let response = InternalSchemeHandler.response(forUrl: model.originalURL)
    return (response, data)
  }
}

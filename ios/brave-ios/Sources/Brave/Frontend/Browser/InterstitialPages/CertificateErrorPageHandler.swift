// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import CertificateUtilities
import Foundation
import Shared

class CertificateErrorPageHandler: InterstitialPageHandler {
  func canHandle(error: NSError) -> Bool {
    return CertificateErrorPageHandler.isValidCertificateError(error: error)
  }

  func response(for model: ErrorPageModel) async -> (URLResponse, Data)? {
    let hasCertificate = model.components.valueForQuery("certerror") != nil

    guard let asset = Bundle.module.url(forResource: "CertificateError", withExtension: "html")
    else {
      assert(false)
      return nil
    }

    guard var html = await AsyncFileManager.default.utf8Contents(at: asset) else {
      assert(false)
      return nil
    }

    var domain = model.domain

    // Update the error code domain
    if domain == kCFErrorDomainCFNetwork as String,
      let code = CFNetworkErrors(rawValue: Int32(model.errorCode))
    {
      domain = GenericErrorPageHandler.CFErrorToName(code)
    } else if domain == NSURLErrorDomain {
      domain = GenericErrorPageHandler.NSURLErrorToName(model.errorCode)
    }

    let host = model.originalURL.normalizedHost(stripWWWSubdomainOnly: true) ?? model.originalHost
    let isBadRoot = model.errorCode == CFNetworkErrors.braveCertificatePinningFailed.rawValue

    var variables = [String: String]()
    if hasCertificate {
      variables = [
        "page_title": host,
        "allow_bypass": "\(!isBadRoot)",
        "error_code": "\(model.errorCode)",
        "error_title": Strings.errorPagesCertWarningTitle,
        "error_description": String(format: Strings.errorPagesAdvancedWarningTitle, host),
        "error_more_details_description": isBadRoot
          ? String(format: Strings.errorPagesAdvancedErrorPinningDetails, host, host, host, host)
          : String(format: Strings.errorPagesAdvancedWarningDetails, host),
        "error_domain": domain,
        "learn_more": Strings.errorPagesLearnMoreButton,
        "more_details": Strings.errorPagesMoreDetailsButton,
        "hide_details": Strings.errorPagesHideDetailsButton,
        "back_to_safety_or_reload": isBadRoot
          ? Strings.errorPageReloadButtonTitle : Strings.errorPagesBackToSafetyButton,
        "visit_unsafe": String(format: Strings.errorPagesProceedAnywayButton, host),
        "has_certificate": "\(hasCertificate)",
        "message_handler": ErrorPageHelper.messageHandlerName,
        "security_token": ErrorPageHelper.scriptId,
        "actions":
          "<button onclick='history.back()'>\(Strings.errorPagesBackToSafetyButton)</button>",
      ]
    } else {
      variables = [
        "page_title": host,
        "allow_bypass": "\(!isBadRoot)",
        "error_code": "\(model.errorCode)",
        "error_title": Strings.errorPagesCertErrorTitle,
        "error_description": model.description,
        "error_more_details_description": "",
        "error_domain": domain,
        "learn_more": "",
        "more_details": "",
        "hide_details": "",
        "back_to_safety_or_reload": "",
        "visit_unsafe": "",
        "has_certificate": "\(hasCertificate)",
        "message_handler": ErrorPageHelper.messageHandlerName,
        "security_token": ErrorPageHelper.scriptId,
        "actions": "",
      ]
    }

    variables.forEach { (arg, value) in
      html = html.replacingOccurrences(of: "%\(arg)%", with: value)
    }

    let data = Data(html.utf8)
    let response = InternalSchemeHandler.response(forUrl: model.originalURL)
    return (response, data)
  }

  static func certsFromErrorURL(_ url: URL) -> [SecCertificate]? {
    func getCerts(_ url: URL) -> [SecCertificate]? {
      let components = URLComponents(url: url, resolvingAgainstBaseURL: false)
      if let encodedCerts = components?.queryItems?.filter({ $0.name == "badcerts" }).first?.value?
        .split(separator: ",")
      {

        return encodedCerts.compactMap({
          guard let certData = Data(base64Encoded: String($0), options: []) else {
            return nil
          }

          return SecCertificateCreateWithData(nil, certData as CFData)
        })
      }

      return nil
    }

    let result = getCerts(url)
    if result != nil {
      return result
    }

    // Fallback case when the error url is nested, this happens when restoring an error url.
    if let internalUrl = InternalURL(url), let url = internalUrl.extractedUrlParam {
      return getCerts(url)
    }
    return nil
  }

  static func isValidCertificateError(error: NSError) -> Bool {
    // Handle CFNetwork Error
    if error.domain == kCFErrorDomainCFNetwork as String,
      let code = CFNetworkErrors(rawValue: Int32(error.code))
    {
      return CFNetworkErrors.certErrors.contains(code)
    }

    // Handle NSURLError
    if error.domain == NSURLErrorDomain as String {
      return NSURLCertErrors.contains(error.code)
    }
    return false
  }
}

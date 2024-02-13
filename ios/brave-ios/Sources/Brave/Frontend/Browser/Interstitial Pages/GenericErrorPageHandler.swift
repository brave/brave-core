// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

class GenericErrorPageHandler: InterstitialPageHandler {
  func canHandle(error: NSError) -> Bool {
    // Handle CFNetwork Error
    if error.domain == kCFErrorDomainCFNetwork as String,
      let code = CFNetworkErrors(rawValue: Int32(error.code)) {

      let unhandledCodes: [CFNetworkErrors] = [
        // Handled by NetworkErrorPageHandler
        .cfurlErrorNotConnectedToInternet,

        // Handled by CertificateErrorPageHandler
        .cfurlErrorSecureConnectionFailed,
        .cfurlErrorServerCertificateHasBadDate,
        .cfurlErrorServerCertificateUntrusted,
        .cfurlErrorServerCertificateHasUnknownRoot,
        .cfurlErrorServerCertificateNotYetValid,
        .cfurlErrorClientCertificateRejected,
        .cfurlErrorClientCertificateRequired,
        .braveCertificatePinningFailed,
      ]

      return !unhandledCodes.contains(code)
    }

    // Handle NSURLError
    if error.domain == NSURLErrorDomain {
      let unhandledCodes: [Int] = [
        // Handled by NetworkErrorPageHandler
        NSURLErrorNotConnectedToInternet,

        // Handled by CertificateErrorPageHandler
        NSURLErrorSecureConnectionFailed,
        NSURLErrorServerCertificateHasBadDate,
        NSURLErrorServerCertificateUntrusted,
        NSURLErrorServerCertificateHasUnknownRoot,
        NSURLErrorServerCertificateNotYetValid,
        NSURLErrorClientCertificateRejected,
        NSURLErrorClientCertificateRequired,
      ]
      return !unhandledCodes.contains(error.code)
    }

    return true
  }

  func response(for model: ErrorPageModel) -> (URLResponse, Data)? {
    guard let asset = Bundle.module.path(forResource: "GenericError", ofType: "html") else {
      assert(false)
      return nil
    }

    guard var html = try? String(contentsOfFile: asset) else {
      assert(false)
      return nil
    }

    var action = ""
    var domain = model.domain
    if domain == kCFErrorDomainCFNetwork as String,
      let code = CFNetworkErrors(rawValue: Int32(model.errorCode)) {

      // Update the error code domain
      domain = GenericErrorPageHandler.CFErrorToName(code)

      // If there are too many redirects, show a `reload` action button
      if code == .cfurlErrorHTTPTooManyRedirects {
        action = """
          <script>
              function reloader() {
                  location.replace((new URL(location.href)).searchParams.get("url"));
              }
          </script>
          <button onclick='reloader()'>\(Strings.errorPageReloadButtonTitle)</button>
          """
      }
    } else if domain == NSURLErrorDomain {
      // Update the error code domain
      domain = GenericErrorPageHandler.NSURLErrorToName(model.errorCode)
    }

    let variables = [
      "page_title": model.originalURL.normalizedHost(stripWWWSubdomainOnly: true) ?? model.originalHost,
      "error_code": "\(model.errorCode)",
      "error_title": "This site can't be reached",
      "error_description": model.description + "<br><br>\(Strings.errorPageCantBeReachedTry)",
      "error_domain": domain,
      "actions": action,
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

  /// Converts `CFNetworkErrors` to a `String`.
  static func CFErrorToName(_ err: CFNetworkErrors) -> String {
    switch err {
    case .cfHostErrorHostNotFound: return "CFHostErrorHostNotFound"
    case .cfHostErrorUnknown: return "CFHostErrorUnknown"
    case .cfsocksErrorUnknownClientVersion: return "CFSOCKSErrorUnknownClientVersion"
    case .cfsocksErrorUnsupportedServerVersion: return "CFSOCKSErrorUnsupportedServerVersion"
    case .cfsocks4ErrorRequestFailed: return "CFSOCKS4ErrorRequestFailed"
    case .cfsocks4ErrorIdentdFailed: return "CFSOCKS4ErrorIdentdFailed"
    case .cfsocks4ErrorIdConflict: return "CFSOCKS4ErrorIdConflict"
    case .cfsocks4ErrorUnknownStatusCode: return "CFSOCKS4ErrorUnknownStatusCode"
    case .cfsocks5ErrorBadState: return "CFSOCKS5ErrorBadState"
    case .cfsocks5ErrorBadResponseAddr: return "CFSOCKS5ErrorBadResponseAddr"
    case .cfsocks5ErrorBadCredentials: return "CFSOCKS5ErrorBadCredentials"
    case .cfsocks5ErrorUnsupportedNegotiationMethod: return "CFSOCKS5ErrorUnsupportedNegotiationMethod"
    case .cfsocks5ErrorNoAcceptableMethod: return "CFSOCKS5ErrorNoAcceptableMethod"
    case .cfftpErrorUnexpectedStatusCode: return "CFFTPErrorUnexpectedStatusCode"
    case .cfErrorHTTPAuthenticationTypeUnsupported: return "CFErrorHTTPAuthenticationTypeUnsupported"
    case .cfErrorHTTPBadCredentials: return "CFErrorHTTPBadCredentials"
    case .cfErrorHTTPConnectionLost: return "CFErrorHTTPConnectionLost"
    case .cfErrorHTTPParseFailure: return "CFErrorHTTPParseFailure"
    case .cfErrorHTTPRedirectionLoopDetected: return "CFErrorHTTPRedirectionLoopDetected"
    case .cfErrorHTTPBadURL: return "CFErrorHTTPBadURL"
    case .cfErrorHTTPProxyConnectionFailure: return "CFErrorHTTPProxyConnectionFailure"
    case .cfErrorHTTPBadProxyCredentials: return "CFErrorHTTPBadProxyCredentials"
    case .cfErrorPACFileError: return "CFErrorPACFileError"
    case .cfErrorPACFileAuth: return "CFErrorPACFileAuth"
    case .cfErrorHTTPSProxyConnectionFailure: return "CFErrorHTTPSProxyConnectionFailure"
    case .cfStreamErrorHTTPSProxyFailureUnexpectedResponseToCONNECTMethod: return "CFStreamErrorHTTPSProxyFailureUnexpectedResponseToCONNECTMethod"

    case .cfurlErrorBackgroundSessionInUseByAnotherProcess: return "CFURLErrorBackgroundSessionInUseByAnotherProcess"
    case .cfurlErrorBackgroundSessionWasDisconnected: return "CFURLErrorBackgroundSessionWasDisconnected"
    case .cfurlErrorUnknown: return "CFURLErrorUnknown"
    case .cfurlErrorCancelled: return "CFURLErrorCancelled"
    case .cfurlErrorBadURL: return "CFURLErrorBadURL"
    case .cfurlErrorTimedOut: return "CFURLErrorTimedOut"
    case .cfurlErrorUnsupportedURL: return "CFURLErrorUnsupportedURL"
    case .cfurlErrorCannotFindHost: return "CFURLErrorCannotFindHost"
    case .cfurlErrorCannotConnectToHost: return "CFURLErrorCannotConnectToHost"
    case .cfurlErrorNetworkConnectionLost: return "CFURLErrorNetworkConnectionLost"
    case .cfurlErrorDNSLookupFailed: return "CFURLErrorDNSLookupFailed"
    case .cfurlErrorHTTPTooManyRedirects: return "CFURLErrorHTTPTooManyRedirects"
    case .cfurlErrorResourceUnavailable: return "CFURLErrorResourceUnavailable"
    case .cfurlErrorNotConnectedToInternet: return "CFURLErrorNotConnectedToInternet"
    case .cfurlErrorRedirectToNonExistentLocation: return "CFURLErrorRedirectToNonExistentLocation"
    case .cfurlErrorBadServerResponse: return "CFURLErrorBadServerResponse"
    case .cfurlErrorUserCancelledAuthentication: return "CFURLErrorUserCancelledAuthentication"
    case .cfurlErrorUserAuthenticationRequired: return "CFURLErrorUserAuthenticationRequired"
    case .cfurlErrorZeroByteResource: return "CFURLErrorZeroByteResource"
    case .cfurlErrorCannotDecodeRawData: return "CFURLErrorCannotDecodeRawData"
    case .cfurlErrorCannotDecodeContentData: return "CFURLErrorCannotDecodeContentData"
    case .cfurlErrorCannotParseResponse: return "CFURLErrorCannotParseResponse"
    case .cfurlErrorInternationalRoamingOff: return "CFURLErrorInternationalRoamingOff"
    case .cfurlErrorCallIsActive: return "CFURLErrorCallIsActive"
    case .cfurlErrorDataNotAllowed: return "CFURLErrorDataNotAllowed"
    case .cfurlErrorRequestBodyStreamExhausted: return "CFURLErrorRequestBodyStreamExhausted"
    case .cfurlErrorFileDoesNotExist: return "CFURLErrorFileDoesNotExist"
    case .cfurlErrorFileIsDirectory: return "CFURLErrorFileIsDirectory"
    case .cfurlErrorNoPermissionsToReadFile: return "CFURLErrorNoPermissionsToReadFile"
    case .cfurlErrorDataLengthExceedsMaximum: return "CFURLErrorDataLengthExceedsMaximum"
    case .cfurlErrorSecureConnectionFailed: return "CFURLErrorSecureConnectionFailed"
    case .cfurlErrorServerCertificateHasBadDate: return "CFURLErrorServerCertificateHasBadDate"
    case .cfurlErrorServerCertificateUntrusted: return "CFURLErrorServerCertificateUntrusted"
    case .cfurlErrorServerCertificateHasUnknownRoot: return "CFURLErrorServerCertificateHasUnknownRoot"
    case .cfurlErrorServerCertificateNotYetValid: return "CFURLErrorServerCertificateNotYetValid"
    case .cfurlErrorClientCertificateRejected: return "CFURLErrorClientCertificateRejected"
    case .cfurlErrorClientCertificateRequired: return "CFURLErrorClientCertificateRequired"
    case .cfurlErrorCannotLoadFromNetwork: return "CFURLErrorCannotLoadFromNetwork"
    case .cfurlErrorCannotCreateFile: return "CFURLErrorCannotCreateFile"
    case .cfurlErrorCannotOpenFile: return "CFURLErrorCannotOpenFile"
    case .cfurlErrorCannotCloseFile: return "CFURLErrorCannotCloseFile"
    case .cfurlErrorCannotWriteToFile: return "CFURLErrorCannotWriteToFile"
    case .cfurlErrorCannotRemoveFile: return "CFURLErrorCannotRemoveFile"
    case .cfurlErrorCannotMoveFile: return "CFURLErrorCannotMoveFile"
    case .cfurlErrorDownloadDecodingFailedMidStream: return "CFURLErrorDownloadDecodingFailedMidStream"
    case .cfurlErrorDownloadDecodingFailedToComplete: return "CFURLErrorDownloadDecodingFailedToComplete"

    case .cfhttpCookieCannotParseCookieFile: return "CFHTTPCookieCannotParseCookieFile"
    case .cfNetServiceErrorUnknown: return "CFNetServiceErrorUnknown"
    case .cfNetServiceErrorCollision: return "CFNetServiceErrorCollision"
    case .cfNetServiceErrorNotFound: return "CFNetServiceErrorNotFound"
    case .cfNetServiceErrorInProgress: return "CFNetServiceErrorInProgress"
    case .cfNetServiceErrorBadArgument: return "CFNetServiceErrorBadArgument"
    case .cfNetServiceErrorCancel: return "CFNetServiceErrorCancel"
    case .cfNetServiceErrorInvalid: return "CFNetServiceErrorInvalid"
    case .cfNetServiceErrorTimeout: return "CFNetServiceErrorTimeout"
    case .cfNetServiceErrorDNSServiceFailure: return "CFNetServiceErrorDNSServiceFailure"
      
    case .braveCertificatePinningFailed: return "ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN"
    default: return "Unknown: \(err.rawValue)"
    }
  }

  /// Converts `NSURLError` to a `String`.
  static func NSURLErrorToName(_ err: Int) -> String {
    switch err {
    case NSURLErrorUnknown: return "NSURLErrorUnknown"
    case NSURLErrorCancelled: return "NSURLErrorCancelled"
    case NSURLErrorBadURL: return "NSURLErrorBadURL"
    case NSURLErrorTimedOut: return "NSURLErrorTimedOut"
    case NSURLErrorUnsupportedURL: return "NSURLErrorUnsupportedURL"
    case NSURLErrorCannotFindHost: return "NSURLErrorCannotFindHost"
    case NSURLErrorCannotConnectToHost: return "NSURLErrorCannotConnectToHost"
    case NSURLErrorNetworkConnectionLost: return "NSURLErrorNetworkConnectionLost"
    case NSURLErrorDNSLookupFailed: return "NSURLErrorDNSLookupFailed"
    case NSURLErrorHTTPTooManyRedirects: return "NSURLErrorHTTPTooManyRedirects"
    case NSURLErrorResourceUnavailable: return "NSURLErrorResourceUnavailable"
    case NSURLErrorNotConnectedToInternet: return "NSURLErrorNotConnectedToInternet"
    case NSURLErrorRedirectToNonExistentLocation: return "NSURLErrorRedirectToNonExistentLocation"
    case NSURLErrorBadServerResponse: return "NSURLErrorBadServerResponse"
    case NSURLErrorUserCancelledAuthentication: return "NSURLErrorUserCancelledAuthentication"
    case NSURLErrorUserAuthenticationRequired: return "NSURLErrorUserAuthenticationRequired"
    case NSURLErrorZeroByteResource: return "NSURLErrorZeroByteResource"
    case NSURLErrorCannotDecodeRawData: return "NSURLErrorCannotDecodeRawData"
    case NSURLErrorCannotDecodeContentData: return "NSURLErrorCannotDecodeContentData"
    case NSURLErrorCannotParseResponse: return "NSURLErrorCannotParseResponse"
    case NSURLErrorAppTransportSecurityRequiresSecureConnection: return "NSURLErrorAppTransportSecurityRequiresSecureConnection"
    case NSURLErrorFileDoesNotExist: return "NSURLErrorFileDoesNotExist"
    case NSURLErrorFileIsDirectory: return "NSURLErrorFileIsDirectory"
    case NSURLErrorNoPermissionsToReadFile: return "NSURLErrorNoPermissionsToReadFile"
    case NSURLErrorDataLengthExceedsMaximum: return "NSURLErrorDataLengthExceedsMaximum"
    case NSURLErrorFileOutsideSafeArea: return "NSURLErrorFileOutsideSafeArea"

    // SSL errors
    case NSURLErrorSecureConnectionFailed: return "NSURLErrorSecureConnectionFailed"
    case NSURLErrorServerCertificateHasBadDate: return "NSURLErrorServerCertificateHasBadDate"
    case NSURLErrorServerCertificateUntrusted: return "NSURLErrorServerCertificateUntrusted"
    case NSURLErrorServerCertificateHasUnknownRoot: return "NSURLErrorServerCertificateHasUnknownRoot"
    case NSURLErrorServerCertificateNotYetValid: return "NSURLErrorServerCertificateNotYetValid"
    case NSURLErrorClientCertificateRejected: return "NSURLErrorClientCertificateRejected"
    case NSURLErrorClientCertificateRequired: return "NSURLErrorClientCertificateRequired"
    case NSURLErrorCannotLoadFromNetwork: return "NSURLErrorCannotLoadFromNetwork"

    // Download and file I/O errors
    case NSURLErrorCannotCreateFile: return "NSURLErrorCannotCreateFile"
    case NSURLErrorCannotOpenFile: return "NSURLErrorCannotOpenFile"
    case NSURLErrorCannotCloseFile: return "NSURLErrorCannotCloseFile"
    case NSURLErrorCannotWriteToFile: return "NSURLErrorCannotWriteToFile"
    case NSURLErrorCannotRemoveFile: return "NSURLErrorCannotRemoveFile"
    case NSURLErrorCannotMoveFile: return "NSURLErrorCannotMoveFile"
    case NSURLErrorDownloadDecodingFailedMidStream: return "NSURLErrorDownloadDecodingFailedMidStream"
    case NSURLErrorDownloadDecodingFailedToComplete: return "NSURLErrorDownloadDecodingFailedToComplete"
    case NSURLErrorInternationalRoamingOff: return "NSURLErrorInternationalRoamingOff"
    case NSURLErrorCallIsActive: return "NSURLErrorCallIsActive"
    case NSURLErrorDataNotAllowed: return "NSURLErrorDataNotAllowed"
    case NSURLErrorRequestBodyStreamExhausted: return "NSURLErrorRequestBodyStreamExhausted"
    case NSURLErrorBackgroundSessionRequiresSharedContainer: return "NSURLErrorBackgroundSessionRequiresSharedContainer"
    case NSURLErrorBackgroundSessionInUseByAnotherProcess: return "NSURLErrorBackgroundSessionInUseByAnotherProcess"
    case NSURLErrorBackgroundSessionWasDisconnected: return "NSURLErrorBackgroundSessionWasDisconnected"
      
    case Int(CFNetworkErrors.braveCertificatePinningFailed.rawValue): return "ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN"
    default: return "Unknown: \(err)"
    }
  }
}

// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

extension CFNetworkErrors {
  static let braveCertificatePinningFailed = CFNetworkErrors(rawValue: Int32.min)!
}

class CertificateErrorPageHandler: InterstitialPageHandler {
  func canHandle(error: NSError) -> Bool {
    return CertificateErrorPageHandler.isValidCertificateError(error: error)
  }

  func response(for model: ErrorPageModel) -> (URLResponse, Data)? {
    let hasCertificate = model.components.valueForQuery("certerror") != nil

    guard let asset = Bundle.module.path(forResource: "CertificateError", ofType: "html") else {
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
    let isBadRoot = model.errorCode == CFNetworkErrors.braveCertificatePinningFailed.rawValue

    var variables = [String: String]()
    if hasCertificate {
      variables = [
        "page_title": host,
        "allow_bypass": "\(!isBadRoot)",
        "error_code": "\(model.errorCode)",
        "error_title": Strings.errorPagesCertWarningTitle,
        "error_description": String(format: Strings.errorPagesAdvancedWarningTitle, host),
        "error_more_details_description": isBadRoot ? String(format: Strings.errorPagesAdvancedErrorPinningDetails, host, host, host, host) : String(format: Strings.errorPagesAdvancedWarningDetails, host),
        "error_domain": domain,
        "learn_more": Strings.errorPagesLearnMoreButton,
        "more_details": Strings.errorPagesMoreDetailsButton,
        "hide_details": Strings.errorPagesHideDetailsButton,
        "back_to_safety_or_reload": isBadRoot ? Strings.errorPageReloadButtonTitle : Strings.errorPagesBackToSafetyButton,
        "visit_unsafe": String(format: Strings.errorPagesProceedAnywayButton, host),
        "has_certificate": "\(hasCertificate)",
        "message_handler": ErrorPageHelper.messageHandlerName,
        "security_token": ErrorPageHelper.scriptId,
        "actions": "<button onclick='history.back()'>\(Strings.errorPagesBackToSafetyButton)</button>",
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

    guard let data = html.data(using: .utf8) else {
      return nil
    }

    let response = InternalSchemeHandler.response(forUrl: model.originalURL)
    return (response, data)
  }

  static func certsFromErrorURL(_ url: URL) -> [SecCertificate]? {
    func getCerts(_ url: URL) -> [SecCertificate]? {
      let components = URLComponents(url: url, resolvingAgainstBaseURL: false)
      if let encodedCerts = components?.queryItems?.filter({ $0.name == "badcerts" }).first?.value?.split(separator: ",") {
        
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

    // Fallback case when the error url is nested, this happens when restoring an error url, it will be inside a 'sessionrestore' url.
    // TODO: Investigate if we can restore directly as an error url and avoid the 'sessionrestore?url=' wrapping.
    if let internalUrl = InternalURL(url), let url = internalUrl.extractedUrlParam {
      return getCerts(url)
    }
    return nil
  }

  static func isValidCertificateError(error: NSError) -> Bool {
    // Handle CFNetwork Error
    if error.domain == kCFErrorDomainCFNetwork as String,
      let code = CFNetworkErrors(rawValue: Int32(error.code)) {
      return CertificateErrorPageHandler.CFNetworkErrorsCertErrors.contains(code)
    }

    // Handle NSURLError
    if error.domain == NSURLErrorDomain as String {
      return CertificateErrorPageHandler.NSURLCertErrors.contains(error.code)
    }
    return false
  }

  // Regardless of cause, cfurlErrorServerCertificateUntrusted is currently returned in all cases.
  // Check the other cases in case this gets fixed in the future.
  // NOTE: In rare cases like Bad Cipher algorithm, it can show cfurlErrorSecureConnectionFailed
  static let CFNetworkErrorsCertErrors: [CFNetworkErrors] = [
    .cfurlErrorSecureConnectionFailed,
    .cfurlErrorServerCertificateHasBadDate,
    .cfurlErrorServerCertificateUntrusted,
    .cfurlErrorServerCertificateHasUnknownRoot,
    .cfurlErrorServerCertificateNotYetValid,
    .cfurlErrorClientCertificateRejected,
    .cfurlErrorClientCertificateRequired,
    .braveCertificatePinningFailed,
  ]

  // Regardless of cause, NSURLErrorServerCertificateUntrusted is currently returned in all cases.
  // Check the other cases in case this gets fixed in the future.
  // NOTE: In rare cases like Bad Cipher algorithm, it can show NSURLErrorSecureConnectionFailed
  static let NSURLCertErrors = [
    NSURLErrorSecureConnectionFailed,
    NSURLErrorServerCertificateHasBadDate,
    NSURLErrorServerCertificateUntrusted,
    NSURLErrorServerCertificateHasUnknownRoot,
    NSURLErrorServerCertificateNotYetValid,
    NSURLErrorClientCertificateRejected,
    NSURLErrorClientCertificateRequired,
    Int(CFNetworkErrors.braveCertificatePinningFailed.rawValue)

    // Apple lists `NSURLErrorCannotLoadFromNetwork` as an `SSL Error`, but I believe the documentation is incorrect as it is for any cache miss.
  ]

  // From: Security.SecBase / <Security/SecureTransport.h>
  // kCFStreamErrorDomainSSL
  static let CertErrorCodes = [
    errSSLProtocol: "SSL_PROTOCOL_ERROR",
    errSSLNegotiation: "SSL_NEGOTIATION_ERROR",
    errSSLFatalAlert: "SSL_FATAL_ERROR",
    errSSLWouldBlock: "SSL_WOULD_BLOCK_ERROR",
    errSSLSessionNotFound: "SSL_SESSION_NOT_FOUND_ERROR",
    errSSLClosedGraceful: "SSL_CLOSED_GRACEFUL_ERROR",
    errSSLClosedAbort: "SSL_CLOSED_ABORT_ERROR",
    errSSLXCertChainInvalid: "SSL_INVALID_CERTIFICATE_CHAIN_ERROR",
    errSSLBadCert: "SSL_BAD_CERTIFICATE_ERROR",
    errSSLCrypto: "SSL_UNDERLYING_CRYPTO_ERROR",
    errSSLInternal: "SSL_INTERNAL_ERROR",
    errSSLModuleAttach: "SSL_MODULE_ATTACH_ERROR",
    errSSLUnknownRootCert: "SSL_VALID_CERT_CHAIN_UNTRUSTED_ROOT_ERROR",
    errSSLNoRootCert: "SSL_NO_ROOT_CERT_ERROR",  // SEC_ERROR_UNKNOWN_ISSUER
    errSSLCertExpired: "SSL_CERTIFICATE_EXPIRED_ERROR",  // SEC_ERROR_EXPIRED_CERTIFICATE
    errSSLCertNotYetValid: "SSL_CERT_IN_CHAIN_NOT_YET_VALID_ERROR",
    errSSLClosedNoNotify: "SSL_SERVER_CLOSED_NO_NOTIFY_ERROR",
    errSSLBufferOverflow: "SSL_BUFFER_OVERFLOW_ERROR",
    errSSLBadCipherSuite: "SSL_BAD_CIPHER_SUITE_ERROR",
    errSSLPeerUnexpectedMsg: "SSL_UNEXPECTED_PEER_MESSAGE_ERROR",
    errSSLPeerBadRecordMac: "SSL_BAD_PEER_RECORD_MAC_ERROR",
    errSSLPeerDecryptionFail: "SSL_PEER_DECRYPTION_FAIL_ERROR",
    errSSLPeerRecordOverflow: "SSL_PEER_RECORD_OVERFLOW_ERROR",
    errSSLPeerDecompressFail: "SSL_PEER_DECOMPRESS_FAIL_ERROR",
    errSSLPeerHandshakeFail: "SSL_PEER_HANDSHAKE_FAIL_ERROR",
    errSSLPeerBadCert: "SSL_PEER_BAD_CERTIFICATE_ERROR",
    errSSLPeerUnsupportedCert: "SSL_PEER_UNSUPPORTED_CERTIFICATE_FORMAT_ERROR",
    errSSLPeerCertRevoked: "SSL_PEER_CERTIFICATE_REVOKED_ERROR",
    errSSLPeerCertExpired: "SSL_PEER_CERTIFICATE_EXPIRED_ERROR",
    errSSLPeerCertUnknown: "SSL_PEER_CERTIFICATE_UNKNOWN_ERROR",
    errSSLIllegalParam: "SSL_ILLEGAL_PARAMETER_ERROR",
    errSSLPeerUnknownCA: "SSL_PEER_UNKNOWN_CERTIFICATE_AUTHORITY_ERROR",
    errSSLPeerAccessDenied: "SSL_PEER_ACCESS_DENIED_ERROR",
    errSSLPeerDecodeError: "SSL_PEER_DECODE_ERROR",
    errSSLPeerDecryptError: "SSL_PEER_DECRYPT_ERROR",
    errSSLPeerExportRestriction: "SSL_PEER_EXPORT_RESTRICTION_ERROR",
    errSSLPeerProtocolVersion: "SSL_PEER_BAD_PROTOCOL_VERSION_ERROR",
    errSSLPeerInsufficientSecurity: "SSL_PEER_INSUFFICIENT_SECURITY_ERROR",
    errSSLPeerInternalError: "SSL_PEER_INTERNAL_ERROR",
    errSSLPeerUserCancelled: "SSL_PEER_USER_CANCELLED_ERROR",
    errSSLPeerNoRenegotiation: "SSL_PEER_NO_RENEGOTIATION_ALLOWED_ERROR",
    errSSLPeerAuthCompleted: "SSL_PEER_AUTH_COMPLETED_ERROR",
    errSSLClientCertRequested: "SSL_CLIENT_CERT_REQUESTED_ERROR",
    errSSLHostNameMismatch: "SSL_HOST_NAME_MISMATCH_ERROR",  // SSL_ERROR_BAD_CERT_DOMAIN
    errSSLConnectionRefused: "SSL_CONNECTION_REFUSED_ERROR",
    errSSLDecryptionFail: "SSL_DECRYPTION_FAIL_ERROR",
    errSSLBadRecordMac: "SSL_BAD_RECORD_MAC_ERROR",
    errSSLRecordOverflow: "SSL_RECORD_OVERFLOW_ERROR",
    errSSLBadConfiguration: "SSL_BAD_CONFIGURATION_ERROR",
    errSSLUnexpectedRecord: "SSL_UNEXPECTED_RECORD_ERROR",
    errSSLWeakPeerEphemeralDHKey: "SSL_WEAK_PEER_EPHEMERAL_DH_KEY_ERROR",
    errSSLClientHelloReceived: "SSL_CLIENT_HELLO_RECEIVED_ERROR",
    errSSLTransportReset: "SSL_TRANSPORT_RESET_ERROR",
    errSSLNetworkTimeout: "SSL_NETWORK_TIMEOUT_ERROR",
    errSSLConfigurationFailed: "SSL_CONFIGURATION_FAILED_ERROR",
    errSSLUnsupportedExtension: "SSL_UNSUPPORTED_EXTENSION_ERROR",
    errSSLUnexpectedMessage: "SSL_UNEXPECTED_MESSAGE_ERROR",
    errSSLDecompressFail: "SSL_DECOMPRESS_FAILED_ERROR",
    errSSLHandshakeFail: "SSL_HANDSHAKE_FAIL_ERROR",
    errSSLDecodeError: "SSL_DECODE_ERROR",
    errSSLInappropriateFallback: "SSL_INAPPROPRIATE_FALLBACK_ERROR",
    errSSLMissingExtension: "SSL_MISSING_EXTENSION_ERROR",
    errSSLBadCertificateStatusResponse: "SSL_BAD_CERTIFICATE_STATUS_RESPONSE_ERROR",
    errSSLCertificateRequired: "SSL_CERTIFICATE_REQUIRED_ERROR",
    errSSLUnknownPSKIdentity: "SSL_UNKNOWN_PSK_IDENTITY_ERROR",
    errSSLUnrecognizedName: "SSL_UNRECOGNIZED_NAME_ERROR",
    errSSLATSViolation: "SSL_ATS_VIOLATION_ERROR",
    errSSLATSMinimumVersionViolation: "SSL_ATS_MINIMUM_VERSION_VIOLATION_ERROR",
    errSSLATSCiphersuiteViolation: "SSL_ATS_CIPHER_SUITE_VIOLATION_ERROR",
    errSSLATSMinimumKeySizeViolation: "SSL_ATS_MINIMUM_KEY_SIZE_VIOLATION_ERROR",
    errSSLATSLeafCertificateHashAlgorithmViolation: "SSL_LEAF_CERT_HASH_ALG_VIOLATION_ERROR",
    errSSLATSCertificateHashAlgorithmViolation: "SSL_ATS_CERT_HASH_ALG_VIOLATION_ERROR",
    errSSLATSCertificateTrustViolation: "SSL_ATS_CERT_TRUST_VIOLATION_ERROR",
    errSSLEarlyDataRejected: "SSL_EARLY_DATA_REJECTION_ERROR",
  ]
}

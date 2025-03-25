// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreFoundation
import Foundation
import Security
import Shared

// Regardless of cause, cfurlErrorServerCertificateUntrusted is currently returned in all cases.
// Check the other cases in case this gets fixed in the future.
// NOTE: In rare cases like Bad Cipher algorithm, it can show cfurlErrorSecureConnectionFailed
// swift-format-ignore
extension CFNetworkErrors {
  public static let braveCertificatePinningFailed = CFNetworkErrors(rawValue: Int32.min)!
  public static let certErrors: [CFNetworkErrors] = [
    .cfurlErrorSecureConnectionFailed,
    .cfurlErrorServerCertificateHasBadDate,
    .cfurlErrorServerCertificateUntrusted,
    .cfurlErrorServerCertificateHasUnknownRoot,
    .cfurlErrorServerCertificateNotYetValid,
    .cfurlErrorClientCertificateRejected,
    .cfurlErrorClientCertificateRequired,
    .braveCertificatePinningFailed,
  ]
}

// Regardless of cause, NSURLErrorServerCertificateUntrusted is currently returned in all cases.
// Check the other cases in case this gets fixed in the future.
// NOTE: In rare cases like Bad Cipher algorithm, it can show NSURLErrorSecureConnectionFailed
// swift-format-ignore
public let NSURLCertErrors = [
  NSURLErrorSecureConnectionFailed,
  NSURLErrorServerCertificateHasBadDate,
  NSURLErrorServerCertificateUntrusted,
  NSURLErrorServerCertificateHasUnknownRoot,
  NSURLErrorServerCertificateNotYetValid,
  NSURLErrorClientCertificateRejected,
  NSURLErrorClientCertificateRequired,
  Int(CFNetworkErrors.braveCertificatePinningFailed.rawValue),

  // Apple lists `NSURLErrorCannotLoadFromNetwork` as an `SSL Error`, but I believe the documentation is incorrect as it is for any cache miss.
]

// From: Security.SecBase / <Security/SecureTransport.h>
// kCFStreamErrorDomainSSL
// swift-format-ignore
public let SecurityCertErrors = [
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

extension InternalURL {
  public var errorCodeForErrorPage: Int? {
    if !isErrorPage {
      return nil
    }
    // ErrorCode is zero if there's no error.
    // Non-Zero (negative or positive) when there is an error

    if InternalURL.isValid(url: url),
      let internalUrl = InternalURL(url),
      internalUrl.isErrorPage
    {

      let query = url.getQuery()
      guard let code = query["code"],
        let errCode = Int(code)
      else {
        return 0
      }

      return errCode
    }
    return 0
  }

  public var certificateErrorForErrorPage: Int? {
    guard let errCode = errorCodeForErrorPage else {
      return nil
    }

    // ErrorCode is zero if there's no error.
    // Non-Zero (negative or positive) when there is an error
    if errCode != 0 {
      if let code = CFNetworkErrors(rawValue: Int32(errCode)),
        CFNetworkErrors.certErrors.contains(code)
      {
        return errCode
      }

      if NSURLCertErrors.contains(errCode) {
        return errCode
      }

      if SecurityCertErrors[OSStatus(errCode)] != nil {
        return errCode
      }
      return 0
    }
    return 0
  }
}

// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Shared

public struct BraveCertificateUtils {
  /// Formats a hex string
  /// Example: formatHex("020401") -> "02 04 01"
  /// Example: formatHex("020401", separator: " - ") -> "02 - 04 - 01"
  public static func formatHex(_ hexString: String, separator: String = " ") -> String {
    let n = 2
    let characters = Array(hexString)

    var result = ""
    stride(from: 0, to: characters.count, by: n).forEach {
      result += String(characters[$0..<min($0 + n, characters.count)])
      if $0 + n < characters.count {
        result += separator
      }
    }
    return result
  }

  private static let dateFormatter: DateFormatter = {
    let dateFormatter = DateFormatter()
    dateFormatter.dateStyle = .full
    dateFormatter.timeStyle = .full
    return dateFormatter
  }()

  public static func formatDate(_ date: Date) -> String {
    return dateFormatter.string(from: date)
  }
}

extension BraveCertificateUtils {
  private enum OIDConversionError: String, Error {
    case tooLarge
    case invalidRootArc
    case invalidBEREncoding
    case invalidLength
    case invalidHexDigits
  }

  /// Converts an ASN1 dot notation OID String to its Binary equivalent
  public static func absoluteOIDToOID(oid: String) throws -> [UInt8] {
    var list = [UInt64]()
    for value in oid.split(separator: ".") {
      if let result = UInt64(value, radix: 10) {
        list.append(result)
      } else {
        // We don't support larger than 64-bits per arc as it would require big-int.
        throw OIDConversionError.tooLarge
      }
    }

    let encodeOctetAsSeptet = { (octet: UInt64) -> [UInt8] in
      var octet = octet
      var encoded = [UInt8]()
      var value = UInt64(0x00)

      while octet >= 0x80 {
        encoded.insert(UInt8((octet & UInt64(0x7F)) | value), at: 0)
        octet >>= 7
        value = 0x80
      }
      encoded.insert(UInt8(octet | value), at: 0)
      return encoded
    }

    if list.count < 2 {
      throw OIDConversionError.invalidBEREncoding
    }

    // Invalid encoding for the root arcs 0 and 1.
    // Invalid encoding the root arc is limited to 0, 1, and 2.
    if (list[0] > 2) || (list[0] <= 1 && list[1] > 39) {
      throw OIDConversionError.invalidRootArc
    }

    var result = encodeOctetAsSeptet(list[0] * 40 + list[1])
    for i in 2..<list.count {
      result.append(contentsOf: encodeOctetAsSeptet(list[i]))
    }

    result.insert(UInt8(result.count), at: 0)
    result.insert(0x06, at: 0)
    return result
  }

  /// Converts a Binary OID to its ASN1 dot notation equivalent
  public static func oidToAbsoluteOID(oid: [UInt8]) throws -> String {
    // Invalid BER encoding
    if oid.count < 2 {
      throw OIDConversionError.invalidBEREncoding
    }

    #if DEBUG
    // Length octet validation
    var length = UInt32(0)
    if (UInt32(oid[1]) & 0x80) != 0x00 {
      length = UInt32(oid[1]) & 0x7F
      if length == 0x00 || length == 0x7F {
        throw OIDConversionError.invalidLength
      }
    } else {
      length = UInt32(oid[1])
      if length == 0x00 {
        throw OIDConversionError.invalidLength
      }
    }

    if (oid.count - 2) != Int(length) {
      throw OIDConversionError.invalidLength
    }
    #endif

    // Drop first 2 octets as it isn't needed for the calculation
    var x = UInt32(oid[2]) / 40
    let y = UInt32(oid[2]) % 40
    var sub = UInt64(0)

    var dotNotation = String()
    if x > 2 {
      x = 2

      dotNotation = "\(x)"
      if (UInt32(oid[2]) & 0x80) != 0x00 {
        sub = 80
      } else {
        dotNotation = ".\(y + ((x - 2) * 40))"
      }
    } else {
      dotNotation = "\(x).\(y)"
    }

    // Drop first 2 octets as it isn't needed for the calculation
    // Start at the next octet
    var value = UInt64(0)
    for i in (sub != 0 ? 2 : 3)..<oid.count {
      value = (value << 7) | (UInt64(oid[i]) & 0x7F)
      if (UInt64(oid[i]) & 0x80) != 0x80 {
        dotNotation += ".\(value - sub)"
        sub = 0
        value = 0
      }
    }
    return dotNotation
  }

  /// Convenience function to convert an ASN1 dot notation OID string to Data (binary)
  public static func absoluteOIDToOID(oid: String) -> Data {
    do {
      let absoluteOID: [UInt8] = try absoluteOIDToOID(oid: oid)
      return Data(bytes: absoluteOID, count: absoluteOID.count)
    } catch {
      return Data()
    }
  }

  /// Convenience function to convert a Binary OID (Data) to its ASN1 dot notation equivalent
  public static func oidToAbsoluteOID(oid: Data) -> String {
    do {
      return try oidToAbsoluteOID(oid: oid.getBytes())
    } catch {
      return String()
    }
  }
}

public enum BraveCertificateUtilError: LocalizedError {
  case noCertificatesProvided
  case cannotCreateServerTrust
  case trustEvaluationFailed

  public var errorDescription: String? {
    switch self {
    case .noCertificatesProvided:
      return "Cannot Create Server Trust - No Certificates Provided"
    case .cannotCreateServerTrust:
      return "Cannot Create Server Trust"
    case .trustEvaluationFailed:
      return "Trust Evaluation Failed"
    }
  }
}

extension BraveCertificateUtils {
  private static let evaluationQueue = DispatchQueue(
    label: "com.brave.cert-utils-evaluation-queue",
    qos: .userInitiated
  )

  public static func createServerTrust(
    _ certificates: [SecCertificate],
    for host: String?
  ) throws -> SecTrust {
    if certificates.isEmpty {
      throw BraveCertificateUtilError.noCertificatesProvided
    }

    var serverTrust: SecTrust?
    let policies = [
      SecPolicyCreateBasicX509(),
      SecPolicyCreateSSL(true, host as CFString?),
    ]

    let status = SecTrustCreateWithCertificates(
      certificates as AnyObject,
      policies as AnyObject,
      &serverTrust
    )
    guard status == errSecSuccess else {
      throw BraveCertificateUtilError.cannotCreateServerTrust
    }
    return serverTrust!
  }

  /// Verifies ServerTrust using Apple's APIs which validates also the X509 Certificate against the System Trusts
  public static func evaluateTrust(_ trust: SecTrust, for host: String?) async throws {
    try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
      BraveCertificateUtils.evaluationQueue.async {
        SecTrustEvaluateAsyncWithError(trust, BraveCertificateUtils.evaluationQueue) {
          _,
          isTrusted,
          error in
          if !isTrusted {
            if let error = error {
              continuation.resume(throwing: error as Error)
            } else {
              continuation.resume(throwing: BraveCertificateUtilError.trustEvaluationFailed)
            }
          } else {
            continuation.resume()
          }
        }
      }
    }
  }

  /// Verifies ServerTrust using Brave-Core which verifies only SSL Pinning Status
  public static func verifyTrust(_ trust: SecTrust, host: String, port: Int) async -> Int {
    return Int(BraveCertificateUtility.verifyTrust(trust, host: host, port: port))
  }
}

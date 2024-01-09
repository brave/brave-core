// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveCore

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

public extension BraveCertificateUtils {
  private enum OIDConversionError: String, Error {
    case tooLarge
    case invalidRootArc
    case invalidBEREncoding
    case invalidLength
    case invalidHexDigits
  }

  /// Converts an ASN1 dot notation OID String to its Binary equivalent
  static func absolute_oid_to_oid(oid: String) throws -> [UInt8] {
    var list = [UInt64]()
    for value in oid.split(separator: ".") {
      if let result = UInt64(value, radix: 10) {
        list.append(result)
      } else {
        // We don't support larger than 64-bits per arc as it would require big-int.
        throw OIDConversionError.tooLarge
      }
    }

    let encode_octet_as_septet = { (octet: UInt64) -> [UInt8] in
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

    var result = encode_octet_as_septet(list[0] * 40 + list[1])
    for i in 2..<list.count {
      result.append(contentsOf: encode_octet_as_septet(list[i]))
    }

    result.insert(UInt8(result.count), at: 0)
    result.insert(0x06, at: 0)
    return result
  }

  /// Converts a Binary OID to its ASN1 dot notation equivalent
  static func oid_to_absolute_oid(oid: [UInt8]) throws -> String {
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
    var X = UInt32(oid[2]) / 40
    let Y = UInt32(oid[2]) % 40
    var sub = UInt64(0)

    var dot_notation = String()
    if X > 2 {
      X = 2

      dot_notation = "\(X)"
      if (UInt32(oid[2]) & 0x80) != 0x00 {
        sub = 80
      } else {
        dot_notation = ".\(Y + ((X - 2) * 40))"
      }
    } else {
      dot_notation = "\(X).\(Y)"
    }

    // Drop first 2 octets as it isn't needed for the calculation
    // Start at the next octet
    var value = UInt64(0)
    for i in (sub != 0 ? 2 : 3)..<oid.count {
      value = (value << 7) | (UInt64(oid[i]) & 0x7F)
      if (UInt64(oid[i]) & 0x80) != 0x80 {
        dot_notation += ".\(value - sub)"
        sub = 0
        value = 0
      }
    }
    return dot_notation
  }

  /// Convenience function to convert an ASN1 dot notation OID string to Data (binary)
  static func absolute_oid_to_oid(oid: String) -> Data {
    do {
      let absolute_oid: [UInt8] = try absolute_oid_to_oid(oid: oid)
      return Data(bytes: absolute_oid, count: absolute_oid.count)
    } catch {
      return Data()
    }
  }

  /// Convenience function to convert a Binary OID (Data) to its ASN1 dot notation equivalent
  static func oid_to_absolute_oid(oid: Data) -> String {
    do {
      return try oid_to_absolute_oid(oid: oid.getBytes())
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

public extension BraveCertificateUtils {
  static func createServerTrust(_ certificates: [SecCertificate], for host: String?) throws -> SecTrust {
    if certificates.isEmpty {
      throw BraveCertificateUtilError.noCertificatesProvided
    }
    
    var serverTrust: SecTrust?
    let policies = [
      SecPolicyCreateBasicX509(),
      SecPolicyCreateSSL(true, host as CFString?),
    ]
    
    let status = SecTrustCreateWithCertificates(certificates as AnyObject,
                                                policies as AnyObject,
                                                &serverTrust)
    guard status == errSecSuccess else {
      throw BraveCertificateUtilError.cannotCreateServerTrust
    }
    return serverTrust!
  }
  
  static func evaluateTrust(_ trust: SecTrust, for host: String?) async throws {
    let policies = [
      SecPolicyCreateSSL(true, host as CFString?),
    ]

    SecTrustSetPolicies(trust, policies as CFTypeRef)
    try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
      let queue = DispatchQueue.global()
      queue.async {
        let result = SecTrustEvaluateAsyncWithError(trust, queue) { _, isTrusted, error in
          if let error = error {
            continuation.resume(throwing: error as Error)
          } else {
            continuation.resume()
          }
        }
        
        if result != errSecSuccess {
          continuation.resume(throwing: BraveCertificateUtilError.trustEvaluationFailed)
        }
      }
    }
  }
  
  static func verifyTrust(_ trust: SecTrust, host: String, port: Int) async -> Int {
    return Int(BraveCertificateUtility.verifyTrust(trust, host: host, port: port))
  }
}

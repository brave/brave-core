// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import DeviceCheck
import BraveCore
import Preferences
import Shared
import os.log

/// A structure used to register a device for Brave's DeviceCheck enrollment
public struct DeviceCheckRegistration: Codable {
  // The enrollment blob is a Base64 Encoded `DeviceCheckEnrollment` structure
  let enrollmentBlob: DeviceCheckEnrollment

  /// The signature is base64(token) + the base64(publicKey) signed using the privateKey.
  let signature: String

  public init(enrollmentBlob: DeviceCheckEnrollment, signature: String) {
    self.enrollmentBlob = enrollmentBlob
    self.signature = signature
  }

  public init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)

    guard let data = Data(base64Encoded: try container.decode(String.self, forKey: .enrollmentBlob)) else {
      throw NSError(
        domain: "com.brave.device.check.enrollment", code: -1,
        userInfo: [
          NSLocalizedDescriptionKey: "Cannot decode enrollmentBlob"
        ])
    }

    enrollmentBlob = try JSONDecoder().decode(DeviceCheckEnrollment.self, from: data)
    signature = try container.decode(String.self, forKey: .signature)
  }

  public func encode(to encoder: Encoder) throws {
    var container = encoder.container(keyedBy: CodingKeys.self)
    let data = try enrollmentBlob.bsonData().base64EncodedString()
    try container.encode(data, forKey: .enrollmentBlob)
    try container.encode(signature, forKey: .signature)
  }

  enum CodingKeys: String, CodingKey {
    case enrollmentBlob
    case signature
  }
}

public struct DeviceCheckEnrollment: Codable {
  // The payment Id from Brave Rewards in UUIDv4 Format.
  let paymentId: String

  // The public key in ASN.1 DER, PEM PKCS#8 Format.
  let publicKey: String

  // The device check token base64 encoded.
  let deviceToken: String

  // Encodes this structure as BSON Format (Binary JSON).
  func bsonData() throws -> Data {
    let formatter = JSONEncoder()
    formatter.outputFormatting = .sortedKeys
    return try formatter.encode(self)
  }
}

/// A structure used to respond to a nonce challenge
public struct AttestationVerifcation: Codable {
  // The attestation blob is a base-64 encoded version of `AttestationBlob`
  let attestationBlob: AttestationBlob

  // The signature is the `nonce` signed by the privateKey and base-64 encoded.
  let signature: String

  public init(attestationBlob: AttestationBlob, signature: String) {
    self.attestationBlob = attestationBlob
    self.signature = signature
  }

  public init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)

    guard let data = Data(base64Encoded: try container.decode(String.self, forKey: .attestationBlob)) else {
      throw NSError(
        domain: "com.brave.device.check.enrollment", code: -1,
        userInfo: [
          NSLocalizedDescriptionKey: "Cannot decode attestationBlob"
        ])
    }

    attestationBlob = try JSONDecoder().decode(AttestationBlob.self, from: data)
    signature = try container.decode(String.self, forKey: .signature)
  }

  public func encode(to encoder: Encoder) throws {
    var container = encoder.container(keyedBy: CodingKeys.self)
    let data = try attestationBlob.bsonData().base64EncodedString()
    try container.encode(data, forKey: .attestationBlob)
    try container.encode(signature, forKey: .signature)
  }

  enum CodingKeys: String, CodingKey {
    case attestationBlob
    case signature
  }
}

public struct AttestationBlob: Codable {
  // The nonce is a UUIDv4 string
  let nonce: String

  // Encodes this structure as BSON Format (Binary JSON).
  func bsonData() throws -> Data {
    let formatter = JSONEncoder()
    formatter.outputFormatting = .sortedKeys
    return try formatter.encode(self)
  }
}

public struct Attestation: Codable {
  // The public key hash is a SHA-256 FingerPrint of the PublicKey in ASN.1 DER format
  let publicKeyHash: String

  // The payment Id from Brave Rewards in UUIDv4 Format.
  let paymentId: String
}

public class DeviceCheckClient {

  // The ID of the private-key stored in the secure-enclave chip
  private static let privateKeyId = "com.brave.device.check.private.key"

  // The current build environment
  private let environment: BraveRewards.Configuration.Environment

  // A structure representing an error returned by the server
  public struct DeviceCheckError: Error, Codable {
    // The error message
    let message: String

    // The http error code
    let code: Int
  }

  public init(environment: BraveRewards.Configuration.Environment = BraveRewards.Configuration.current().environment) {
    self.environment = environment
  }

  public class func isDeviceEnrolled() -> Bool {
    let hasPrivateKey = Cryptography.keyExists(id: DeviceCheckClient.privateKeyId)
    let didEnrollSuccessfully = Preferences.Rewards.didEnrollDeviceCheck.value
    return hasPrivateKey && didEnrollSuccessfully
  }

  public class func resetDeviceEnrollment() {
    Preferences.Rewards.didEnrollDeviceCheck.value = false
    if let error = Cryptography.delete(id: DeviceCheckClient.privateKeyId) {
      Logger.module.error("\(error.localizedDescription)")
    }
  }

  // MARK: - Server calls for DeviceCheck

  // Registers a device with the server using the device-check token
  public func registerDevice(enrollment: DeviceCheckRegistration) async throws {
    let _: Data = try await executeRequest(.register(enrollment))
    Preferences.Rewards.didEnrollDeviceCheck.value = true
  }

  // Retrieves existing attestations for this device and returns a nonce if any
  public func getAttestation(attestation: Attestation) async throws -> AttestationBlob {
    return try await executeRequest(.getAttestation(attestation))
  }

  // Sends the attestation to the server along with the nonce and the challenge signature
  public func setAttestation(nonce: String, verification: AttestationVerifcation) async throws {
    let _: Data = try await executeRequest(.setAttestation(nonce, verification))
  }
  
  public func solveAdaptiveCaptcha(
    paymentId: String,
    captchaId: String,
    verification: AttestationVerifcation
  ) async throws {
    let _: Data = try await executeRequest(.solveCaptcha(paymentId: paymentId, captchaId: captchaId, nonce: verification.attestationBlob.nonce))
  }

  // MARK: - Factory functions for generating structures to be used with the above server calls

  // Generates a device-check token
  public func generateToken(_ completion: @escaping (String, Error?) -> Void) {
    DCDevice.current.generateToken { data, error in
      if let error = error {
        return completion("", error)
      }

      guard let deviceCheckToken = data?.base64EncodedString() else {
        return completion("", error)
      }

      completion(deviceCheckToken, nil)
    }
  }

  // Generates an enrollment structure to be used with `registerDevice`
  public func generateEnrollment(paymentId: String, token: String) throws -> DeviceCheckRegistration {
    guard let privateKey = try Cryptography.generateKey(id: DeviceCheckClient.privateKeyId) else {
      throw CryptographyError(description: "Unable to generate private key")
    }
    
    guard let publicKey = try privateKey.getPublicAsPEM() else {
      throw CryptographyError(description: "Unable to retrieve public key")
    }
    
    let enrollment = DeviceCheckEnrollment(
      paymentId: paymentId,
      publicKey: publicKey,
      deviceToken: token)
    
    let signature = try privateKey.sign(message: enrollment.bsonData()).base64EncodedString()
    
    let registration = DeviceCheckRegistration(
      enrollmentBlob: enrollment,
      signature: signature)
    return registration
  }

  // Generates an attestation structure to be used with `getAttestation`
  public func generateAttestation(paymentId: String) throws -> Attestation {
    guard let privateKey = try Cryptography.getExistingKey(id: DeviceCheckClient.privateKeyId) else {
      throw CryptographyError(description: "Unable to retrieve existing private key")
    }
    
    guard let publicKeyFingerprint = try privateKey.getPublicKeySha256FingerPrint() else {
      throw CryptographyError(description: "Unable to retrieve public key")
    }
    
    let attestation = Attestation(
      publicKeyHash: publicKeyFingerprint,
      paymentId: paymentId)
    
    return attestation
  }

  // Generates an attestation verification structure to be used with `setAttestation`
  public func generateAttestationVerification(nonce: String) throws -> AttestationVerifcation {
    guard let privateKey = try Cryptography.getExistingKey(id: DeviceCheckClient.privateKeyId) else {
      throw CryptographyError(description: "Unable to retrieve existing private key")
    }
    
    let attestation = AttestationBlob(nonce: nonce)
    let signature = try privateKey.sign(message: try attestation.bsonData()).base64EncodedString()
    let verification = AttestationVerifcation(
      attestationBlob: attestation,
      signature: signature)
    
    return verification
  }
}

private extension DeviceCheckClient {
  // The base URL of the server
  private var baseURL: URL? {
    switch environment {
    case .development:
      return URL(string: "https://grant.rewards.brave.software")
    case .staging:
      return URL(string: "https://grant.rewards.bravesoftware.com")
    case .production:
      return URL(string: "https://grant.rewards.brave.com")
    }
  }

  private enum HttpMethod: String {
    case get
    case put
    case post
  }

  private enum Request {
    case register(DeviceCheckRegistration)
    case getAttestation(Attestation)
    case setAttestation(String, AttestationVerifcation)
    case solveCaptcha(paymentId: String, captchaId: String, nonce: String)

    func method() -> HttpMethod {
      switch self {
      case .register: return .post
      case .getAttestation: return .post
      case .setAttestation: return .put
      case .solveCaptcha: return .post
      }
    }

    func url(for client: DeviceCheckClient) -> URL? {
      switch self {
      case .register:
        return URL(string: "/v1/devicecheck/enrollments", relativeTo: client.baseURL)

      case .getAttestation:
        return URL(string: "/v1/devicecheck/attestations", relativeTo: client.baseURL)

      case .setAttestation(let nonce, _):
        let nonce = nonce.addingPercentEncoding(withAllowedCharacters: .urlPathAllowed) ?? nonce
        return URL(string: "/v1/devicecheck/attestations/\(nonce)", relativeTo: client.baseURL)
      case .solveCaptcha(let paymentId, let captchaId, _):
        let paymentId = paymentId.addingPercentEncoding(withAllowedCharacters: .urlPathAllowed) ?? paymentId
        let captchaId = captchaId.addingPercentEncoding(withAllowedCharacters: .urlPathAllowed) ?? captchaId
        return URL(string: "/v3/captcha/solution/\(paymentId)/\(captchaId)", relativeTo: client.baseURL)
      }
    }
  }

  private func executeRequest<T: Codable>(_ request: Request) async throws -> T {
    let request = try encodeRequest(request)
    return try await withCheckedThrowingContinuation { continuation in
      let task = URLSession(configuration: .ephemeral, delegate: nil, delegateQueue: .main).dataTask(with: request) { data, response, error in
        
        if let error = error {
          continuation.resume(throwing: error)
          return
        }
        
        if let data = data, let error = try? JSONDecoder().decode(DeviceCheckError.self, from: data) {
          continuation.resume(throwing: error)
          return
        }
        
        if let response = response as? HTTPURLResponse {
          if response.statusCode < 200 || response.statusCode > 299 {
            let error = DeviceCheckError(
              message: "Validation Failed: Invalid Response Code",
              code: response.statusCode
            )
            continuation.resume(throwing: error)
            return
          }
        }
        
        guard let data = data else {
          continuation.resume(throwing:
            DeviceCheckError(message: "Validation Failed: Empty Server Response", code: 500)
          )
          return
        }
        
        if T.self == Data.self {
          continuation.resume(returning: data as! T) // swiftlint:disable:this force_cast
          return
        }
        
        if T.self == String.self {
          continuation.resume(returning: String(data: data, encoding: .utf8) as! T) // swiftlint:disable:this force_cast
          return
        }
        
        do {
          let value = try JSONDecoder().decode(T.self, from: data)
          continuation.resume(returning: value)
        } catch {
          continuation.resume(throwing: error)
        }
      }
      task.resume()
    }
  }

  // Encodes the given `endpoint` into a `URLRequest
  private func encodeRequest(_ endpoint: Request) throws -> URLRequest {
    guard let url = endpoint.url(for: self) else {
      throw DeviceCheckError(message: "Invalid URL for Request", code: 400)
    }

    var request = URLRequest(url: url)
    request.httpMethod = endpoint.method().rawValue
    request.setValue("application/json", forHTTPHeaderField: "Accept")

    switch endpoint {
    case .register(let parameters):
      request.httpBody = try JSONEncoder().encode(parameters)
      request.setValue("application/json", forHTTPHeaderField: "Content-Type")

    case .getAttestation(let parameters):
      request.httpBody = try JSONEncoder().encode(parameters)
      request.setValue("application/json", forHTTPHeaderField: "Content-Type")

    case .setAttestation(_, let parameters):
      request.httpBody = try JSONEncoder().encode(parameters)
      request.setValue("application/json", forHTTPHeaderField: "Content-Type")
      
    case .solveCaptcha(_, _, let nonce):
      request.httpBody = try JSONSerialization.data(withJSONObject: ["solution": nonce])
      request.setValue("application/json", forHTTPHeaderField: "Content-Type")
    }
    return request
  }
}

private extension DeviceCheckClient {

  // Encodes parameters into the query component of the URL
  private func encodeQueryURL(url: URL, parameters: [String: String]) -> URL? {
    var urlComponents = URLComponents()
    urlComponents.scheme = url.scheme
    urlComponents.host = url.host
    urlComponents.path = url.path
    urlComponents.queryItems = parameters.map({
      URLQueryItem(name: $0.key, value: $0.value)
    })
    return urlComponents.url
  }
}

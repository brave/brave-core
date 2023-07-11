// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import CertificateUtilities
@testable import BraveCore

class CertificateUtilsTest: XCTestCase {
  private func certificate(named: String) -> SecCertificate {
    let path = Bundle.module.path(forResource: named, ofType: ".cer")!
    let certificateData = try! Data(contentsOf: URL(fileURLWithPath: path)) as CFData
    return SecCertificateCreateWithData(nil, certificateData)!
  }

  func testHexFormatting() {
    XCTAssertEqual(BraveCertificateUtils.formatHex("AABBCCDDEEFF"), "AA BB CC DD EE FF")
    XCTAssertEqual(BraveCertificateUtils.formatHex("AABBCCDDEEFF", separator: "-"), "AA-BB-CC-DD-EE-FF")
    XCTAssertEqual(BraveCertificateUtils.formatHex("AABBCCDDEEFF", separator: ""), "AABBCCDDEEFF")
  }

  func testAbsolute_OID_To_OID() {
    let testOIDs: [String: [UInt8]] = [
      "0": [], /* Invalid format */
      "1.1": [0x06, 0x01, 0x29],
      "X.YY.ZZZZ": [], /* Invalid format */
      "0.40.9999": [], /* Invalid arc-2: 40 */
      "0.35.9999": [0x06, 0x03, 0x23, 0xCE, 0x0F],
      "1.35.9999": [0x06, 0x03, 0x4B, 0xCE, 0x0F],
      "1.40.9999": [], /* Invalid arc-2: 40 */
      "1.999.9999": [], /* Invalid arc-2: 999 */
      "1.30.999999999999999999999999999": [], /* Larger than 64-bit UInt per arc */
      "2.999.9999": [0x06, 0x04, 0x88, 0x37, 0xCE, 0x0F],
      "3.999.9999": [], /* Invalid arc-1: 3 */
      "1.2.840.113549.1.1.11": [0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B], /* SHA256 with RSA Encryption */
      "1.2.840.10045.4.3.2": [0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02], /* ECDSA Signature with SHA-256 */
      "1.2.840.10045.2.1": [0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01], /* Elliptic Curve Public Key */
      "1.2.840.10045.3.1.7": [0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07] /* Elliptic Curve secp256r1 */,
    ]

    for test in testOIDs {
      do {
        let result: [UInt8] = try BraveCertificateUtils.absolute_oid_to_oid(oid: test.key)
        XCTAssertEqual(test.value, result, test.key)
      } catch {
        XCTAssertEqual(test.value, [], test.key)
      }
    }
  }

  func testOID_To_Absolute_OID() {
    let testOIDs: [[UInt8]: String] = [
      []: "", /* Invalid format */
      [0x06, 0x03, 0x88, 0x37, 0xCE, 0x0F]: "", /* Invalid length of 0x03, 4 bytes following */
      [0x06, 0x01, 0x29]: "1.1",
      [0x06, 0x03, 0x28, 0xCE, 0x0F]: "1.0.9999", /* Invalid arc-2: 40 */
      [0x06, 0x03, 0x23, 0xCE, 0x0F]: "0.35.9999",
      [0x06, 0x03, 0x4B, 0xCE, 0x0F]: "1.35.9999",
      [0x06, 0x04, 0x88, 0x37, 0xCE, 0x0F]: "2.999.9999",
      [0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B]: "1.2.840.113549.1.1.11",
      [0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02]: "1.2.840.10045.4.3.2",
      [0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01]: "1.2.840.10045.2.1",
      [0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07]: "1.2.840.10045.3.1.7",
    ]

    for test in testOIDs {
      do {
        let result: String = try BraveCertificateUtils.oid_to_absolute_oid(oid: test.key)
        XCTAssertEqual(test.value, result, test.value)
      } catch {
        XCTAssertEqual(test.value, "", test.value)
      }
    }
  }

  func testCertificateInfo() {
    guard let model = BraveCertificateModel(certificate: certificate(named: "github.com")) else {
      XCTAssert(false, "Cannot load Certificate")
      return
    }

    XCTAssertEqual(model.isRootCertificate, false)
    XCTAssertEqual(model.isCertificateAuthority, false)
    XCTAssertEqual(model.isSelfSigned, false)
    XCTAssertEqual(model.isSelfIssued, false)

    XCTAssertEqual(model.subjectName.countryOrRegion, "US")
    XCTAssertEqual(model.subjectName.stateOrProvince, "California")
    XCTAssertEqual(model.subjectName.locality, "San Francisco")
    XCTAssertEqual(model.subjectName.organization, ["GitHub, Inc."])
    XCTAssertEqual(model.subjectName.organizationalUnit, [])
    XCTAssertEqual(model.subjectName.commonName, "github.com")
    XCTAssertEqual(model.subjectName.userId, "")

    XCTAssertEqual(model.issuerName.countryOrRegion, "US")
    XCTAssertEqual(model.issuerName.stateOrProvince, "")
    XCTAssertEqual(model.issuerName.locality, "")
    XCTAssertEqual(model.issuerName.organization, ["DigiCert, Inc."])
    XCTAssertEqual(model.issuerName.organizationalUnit, [])
    XCTAssertEqual(model.issuerName.commonName, "DigiCert High Assurance TLS Hybrid ECC SHA256 2020 CA1")
    XCTAssertEqual(model.issuerName.userId, "")

    XCTAssertEqual(model.serialNumber, "0E8BF3770D92D196F0BB61F93C4166BE")
    XCTAssertEqual(model.version, 3)

    XCTAssertEqual(model.signature.algorithm, "ECDSA")
    XCTAssertEqual(model.signature.objectIdentifier.isEmpty, true)
    XCTAssertEqual(model.signature.absoluteObjectIdentifier, "1.2.840.10045.4.3.2")
    XCTAssertEqual(model.signature.parameters, "")

    XCTAssertEqual(model.notValidBefore.timeIntervalSince1970, 1616630400.0)
    XCTAssertEqual(model.notValidAfter.timeIntervalSince1970, 1648684799.0)

    XCTAssertEqual(model.publicKeyInfo.type, .EC)
    XCTAssertEqual(model.publicKeyInfo.algorithm, "Elliptic Curve (Prime Random)")
    XCTAssertEqual(model.publicKeyInfo.curveName, "")
    XCTAssertEqual(model.publicKeyInfo.nistCurveName, "")
    XCTAssertEqual(model.publicKeyInfo.absoluteObjectIdentifier, "1.2.840.10045.2.1")
    XCTAssertEqual(model.publicKeyInfo.parameters, "06082A8648CE3D030107")
    XCTAssertEqual(model.publicKeyInfo.keyHexEncoded, "04ADF6F775B1D349540A5D1071BDDC25064B221CA2234E9FA1FEB9D08CBD39BC0C23C7CF91A6905AD845AB0313BEC1237AB9C4C89D47F696E0B9766B503666F70D")
    XCTAssertEqual(model.publicKeyInfo.effectiveSize, 256)
    XCTAssertEqual(model.publicKeyInfo.keySizeInBits, 256)
    XCTAssertEqual(model.publicKeyInfo.keyBytesSize, 32)
    XCTAssertEqual(model.publicKeyInfo.exponent, 0)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.ENCRYPT), true)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.VERIFY), true)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.DERIVE), false)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.DECRYPT), false)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.SIGN), false)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.WRAP), true)

    XCTAssertEqual(model.sha1Fingerprint.fingerprintHexEncoded, "8463B3A92912CCFD1D314705989BEC139937D0D7")
    XCTAssertEqual(model.sha256Fingerprint.fingerprintHexEncoded, "0AE384BFD4DDE9D13E50C5857C05A442C93F8E01445EE4B34540D22BD1E37F1B")
  }

  func testCertificateInfo2() {
    guard let model = BraveCertificateModel(certificate: certificate(named: "brave.com")) else {
      XCTAssert(false, "Cannot load Certificate")
      return
    }

    XCTAssertEqual(model.isRootCertificate, false)
    XCTAssertEqual(model.isCertificateAuthority, false)
    XCTAssertEqual(model.isSelfSigned, false)
    XCTAssertEqual(model.isSelfIssued, false)

    XCTAssertEqual(model.subjectName.countryOrRegion, "")
    XCTAssertEqual(model.subjectName.stateOrProvince, "")
    XCTAssertEqual(model.subjectName.locality, "")
    XCTAssertEqual(model.subjectName.organization, [])
    XCTAssertEqual(model.subjectName.organizationalUnit, [])
    XCTAssertEqual(model.subjectName.commonName, "brave.com")
    XCTAssertEqual(model.subjectName.userId, "")

    XCTAssertEqual(model.issuerName.countryOrRegion, "US")
    XCTAssertEqual(model.issuerName.stateOrProvince, "")
    XCTAssertEqual(model.issuerName.locality, "")
    XCTAssertEqual(model.issuerName.organization, ["Amazon"])
    XCTAssertEqual(model.issuerName.organizationalUnit, ["Server CA 1B"])
    XCTAssertEqual(model.issuerName.commonName, "Amazon")
    XCTAssertEqual(model.issuerName.userId, "")

    XCTAssertEqual(model.serialNumber, "0F03E5078438C49822F3A852EBC02254")
    XCTAssertEqual(model.version, 3)

    XCTAssertEqual(model.signature.algorithm, "RSA")
    XCTAssertEqual(model.signature.objectIdentifier.isEmpty, true)
    XCTAssertEqual(model.signature.absoluteObjectIdentifier, "1.2.840.113549.1.1.11")
    XCTAssertEqual(model.signature.parameters, "")

    XCTAssertEqual(model.notValidBefore.timeIntervalSince1970, 1642464000.0)
    XCTAssertEqual(model.notValidAfter.timeIntervalSince1970, 1676591999.0)

    XCTAssertEqual(model.publicKeyInfo.type, .RSA)
    XCTAssertEqual(model.publicKeyInfo.algorithm, "RSA")
    XCTAssertEqual(model.publicKeyInfo.curveName, "")
    XCTAssertEqual(model.publicKeyInfo.nistCurveName, "")
    XCTAssertEqual(model.publicKeyInfo.absoluteObjectIdentifier, "1.2.840.113549.1.1.1")
    XCTAssertEqual(model.publicKeyInfo.parameters, "")
    XCTAssertEqual(model.publicKeyInfo.keyHexEncoded, "00EDB1B4224D94945B13175712561C21108FA71FD99CDA01D5F60860941D6CC793B64F3F470B956BAA606891F30C9EEB789EB4EEDFB817280405093BAA9E62F9C0B4C3B87424F2DF443E2ACD1F95B634530824E3FCAF4D9178522C8FC2B5F32158B9EF3F6AA281E52C57A6B9FBF0D4C7BED4EA1633EAA1AF2F8FE84EA3FCF80B4566E79026078C10D64AF3E67AB9D302220038DB2B026637DE42A854F639F0C3D9742EA7005BA49984AA81F897A36F36082E9FD44ADC92DC88744ABF6028D6C39DBEF1DB1A086F50AF8E35F5757A443F3D5444794837716A07A806B7F9EAD1E551A2ABCBD0ECB59BEAC0F685222CE577C0EBF5248693CAD197B3A9BD701482711F")
    XCTAssertEqual(model.publicKeyInfo.effectiveSize, 2048)
    XCTAssertEqual(model.publicKeyInfo.keySizeInBits, 2048)
    XCTAssertEqual(model.publicKeyInfo.keyBytesSize, 256)
    XCTAssertEqual(model.publicKeyInfo.exponent, 65537)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.ENCRYPT), true)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.VERIFY), true)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.DERIVE), false)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.DECRYPT), true)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.SIGN), false)
    XCTAssertEqual(model.publicKeyInfo.keyUsage.contains(.WRAP), true)

    XCTAssertEqual(model.sha1Fingerprint.fingerprintHexEncoded, "BAA20BC0E5A5885974F2050CC6D1F7A206C0F945")
    XCTAssertEqual(model.sha256Fingerprint.fingerprintHexEncoded, "D3C09366E951BF0F1D604D5DCD74B6F8FAB8B85ABDC96401D61816CE09740D07")
  }
}

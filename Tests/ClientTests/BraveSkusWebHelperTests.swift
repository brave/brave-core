// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Brave

final class BraveSkusWebHelperTests: XCTestCase {
  
  func testShouldInjectReceipt() throws {
    
    let goodURLs: [URL?] =
    [.init(string: "https://account.brave.com/?intent=connect-receipt&product=vpn"),
     .init(string: "https://account.brave.software/?intent=connect-receipt&product=vpn"),
     .init(string: "https://account.bravesoftware.com?intent=connect-receipt&product=vpn"),
     // One extra query param
     .init(string: "https://account.bravesoftware.com?intent=connect-receipt&product=vpn&extra_param=test"),
     // Url path agnostic test
     .init(string: "https://account.brave.com/extrapath?intent=connect-receipt&product=vpn")]
    let badURLs: [URL?] =
    [  // Wrong host, correct query params
      .init(string: "https://example.com/?intent=connect-receipt&product=vpn"),
      // Correct host, no query params
      .init(string: "https://account.brave.com/"),
      // Correct host, single query param,
      .init(string: "https://account.brave.com/?intent=connect-receipt"),
      .init(string: "https://account.brave.com/?product=vpn"),
      // Correct host, incorrect query param values
      .init(string: "https://account.brave.com/?intent=bad&product=not_vpn"),
      // Http connection
      .init(string: "http://account.brave.com/?intent=connect-receipt&product=vpn"),
    ]
    
    try goodURLs.forEach {
      XCTAssertNotNil(BraveSkusWebHelperMock(for: try XCTUnwrap($0)), "failed at: \($0?.absoluteString ?? "nil")")
    }
    
    try badURLs.forEach {
      XCTAssertNil(BraveSkusWebHelperMock(for: try XCTUnwrap($0)), "failed at: \($0?.absoluteString ?? "nil")")
    }
  }
  
  func testReceiptJson() throws {
    let url = try XCTUnwrap(URL(string: "https://account.brave.com/?intent=connect-receipt&product=vpn"))
    
    let helper = try XCTUnwrap(BraveSkusWebHelperMock(for: url))
    let receiptData = try XCTUnwrap(helper.receiptData)
    XCTAssertEqual(receiptData.key, "braveVpn.receipt")
    let value = receiptData.value
    let decodedData = try XCTUnwrap(Data(base64Encoded: value))
    
    let json = try XCTUnwrap(JSONSerialization.jsonObject(with: decodedData) as? [String: String])
    let type = try XCTUnwrap(json["type"])
    let rawReceipt = try XCTUnwrap(json["raw_receipt"])
    let package = try XCTUnwrap(json["package"])
    let subscriptionId = try XCTUnwrap(json["subscription_id"])
    
    XCTAssertEqual(type, "ios")
    XCTAssertEqual(rawReceipt, BraveSkusWebHelperMock.mockReceiptValue)
    XCTAssertEqual(package, try XCTUnwrap(Bundle.main.bundleIdentifier))
    XCTAssertEqual(subscriptionId, "brave-firewall-vpn-premium")
  }
}

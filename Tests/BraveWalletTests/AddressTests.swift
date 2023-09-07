// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveWallet

class AddressTests: XCTestCase {
  func testAddressTruncation() {
    let address = "0xabcdef0123456789"
    XCTAssertEqual(address.truncatedAddress, "0xabcd…6789")

    let prefixlessAddress = "abcdef0123456789"
    XCTAssertEqual(prefixlessAddress.truncatedAddress, "abcd…6789")
  }

  func testRemovingHexPrefix() {
    let address = "0xabcdef0123456789"
    XCTAssertEqual(address.removingHexPrefix, "abcdef0123456789")

    let prefixlessAddress = "abcdef0123456789"
    XCTAssertEqual(prefixlessAddress.removingHexPrefix, "abcdef0123456789")
  }

  func testIsETHAddress() {
    let isAddressTrue = "0x0c84cD05f2Bc2AfD7f29d4E71346d17697C353b7"
    XCTAssertTrue(isAddressTrue.isETHAddress)

    let isAddressFalseNotHex = "0x0csadgasg"
    XCTAssertFalse(isAddressFalseNotHex.isETHAddress)

    let isAddressFalseWrongPrefix = "0c84cD05f2Bc2AfD7f29d4E71346d17697C353b7"
    XCTAssertFalse(isAddressFalseWrongPrefix.isETHAddress)

    let isAddressFalseTooShort = "0x84cD05f2"
    XCTAssertFalse(isAddressFalseTooShort.isETHAddress)

    let isAddressFalseNoHexDigits = "0x"
    XCTAssertFalse(isAddressFalseNoHexDigits.isETHAddress)
  }
  
  func testStrippedETHAddress() {
    let address = "0x0c84cD05f2Bc2AfD7f29d4E71346d17697C353b7"
    XCTAssertEqual(address, address.strippedETHAddress)
    
    let addressEthereumPrefix = "ethereum:0x0c84cD05f2Bc2AfD7f29d4E71346d17697C353b7"
    let addressEthereumPrefixStripped = addressEthereumPrefix.strippedETHAddress
    XCTAssertNotEqual(addressEthereumPrefix, addressEthereumPrefixStripped)
    XCTAssertEqual(addressEthereumPrefixStripped, address)
    
    let addressEthereumPrefix2 = "Ethereum:0x0c84cD05f2Bc2AfD7f29d4E71346d17697C353b7"
    let addressEthereumPrefix2Stripped = addressEthereumPrefix2.strippedETHAddress
    XCTAssertNotEqual(addressEthereumPrefix2, addressEthereumPrefix2Stripped)
    XCTAssertEqual(addressEthereumPrefix2Stripped, address)
    
    let addressEthPrefix = "eth:0x0c84cD05f2Bc2AfD7f29d4E71346d17697C353b7"
    let addressEthPrefixStripped = addressEthPrefix.strippedETHAddress
    XCTAssertNotEqual(addressEthPrefix, addressEthPrefixStripped)
    XCTAssertEqual(addressEthPrefixStripped, address)
    
    let addressFalseWrongPrefix = "0c84cD05f2Bc2AfD7f29d4E71346d17697C353b7"
    let addressFalseWrongPrefixStripped = addressFalseWrongPrefix.strippedETHAddress
    XCTAssertEqual(addressFalseWrongPrefix, addressFalseWrongPrefixStripped)
  }
  
  func testStrippedSOLAddress() {
    let address = "DpF8LrdYWH9jCsmjQfKz3cvWvRWQPS3xrdVLm8qUs2ZL"
    XCTAssertEqual(address, address.strippedETHAddress)
    
    let addressSolanaPrefix = "solana:\(address)"
    let addressSolanaPrefixStripped = addressSolanaPrefix.strippedSOLAddress
    XCTAssertNotEqual(addressSolanaPrefix, addressSolanaPrefixStripped)
    XCTAssertEqual(addressSolanaPrefixStripped, address)
    
    let addressSolanaPrefix2 = "Solana:\(address)"
    let addressSolanaPrefix2Stripped = addressSolanaPrefix2.strippedSOLAddress
    XCTAssertNotEqual(addressSolanaPrefix2, addressSolanaPrefix2Stripped)
    XCTAssertEqual(addressSolanaPrefix2Stripped, address)
  }

  func testZwspOutput() {
    let address = "0x1bBE4E6EF7294c99358377abAd15A6d9E98127A2"
    let zwspAddress = "0\u{200b}x\u{200b}1\u{200b}b\u{200b}B\u{200b}E\u{200b}4\u{200b}E\u{200b}6\u{200b}E\u{200b}F\u{200b}7\u{200b}2\u{200b}9\u{200b}4\u{200b}c\u{200b}9\u{200b}9\u{200b}3\u{200b}5\u{200b}8\u{200b}3\u{200b}7\u{200b}7\u{200b}a\u{200b}b\u{200b}A\u{200b}d\u{200b}1\u{200b}5\u{200b}A\u{200b}6\u{200b}d\u{200b}9\u{200b}E\u{200b}9\u{200b}8\u{200b}1\u{200b}2\u{200b}7\u{200b}A\u{200b}2\u{200b}"
    let result = address.zwspOutput
    XCTAssertNotEqual(address, result)
    XCTAssertEqual(zwspAddress, result)
  }
  
  func testIsFILAddress() {
    let isMainnetAddressTrue = "f1ysqn2zflyeb1jqqi2bqbomgjtodunoplkfedbpa"
    XCTAssertTrue(isMainnetAddressTrue.isFILAddress)
    
    let isTestnetAddressTrue = "t165quq7gkjh6ebshr7qi2ud7vycel4m7x6dvfvga"
    XCTAssertTrue(isTestnetAddressTrue.isFILAddress)
    
    let isAddressFalseWrongPrefix = "a1ysqn2zflyeb1jqqi2bqbomgjtodunoplkfedbpa"
    XCTAssertFalse(isAddressFalseWrongPrefix.isFILAddress)
    
    let isAddressFalseCorrentLength1 = "f1ysqn2zflyeb1jqqi2bqbomgjtodunoplkfedbpa"
    XCTAssertTrue(isAddressFalseCorrentLength1.isFILAddress)
    
    let isAddressFalseCorrentLength2 = "f1ysqn2zflyeb1jqqi2bqbomgjtodunoplkfedbpa2ba"
    XCTAssertTrue(isAddressFalseCorrentLength2.isFILAddress)
    
    let isAddressFalseCorrentLength3 = "f1ysqn2zflyeb1jqqi2bqbomgjtodunoplkfedbpaf1ysqn2zflyeb1jqqi2bqbomgjtodunoplkfedbpa1gdu"
    XCTAssertTrue(isAddressFalseCorrentLength3.isFILAddress)
    
    let isAddressFalseWrongLength = "f1ysqn2zflyeb1jqqi2bqbomgjtodundbpa"
    XCTAssertFalse(isAddressFalseWrongLength.isFILAddress)
  }
}

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Data
import BraveShared
import JavaScriptCore

class SyncCryptoTests: CoreDataTestCase {
    
    var sc: SyncCrypto!
    
    // Example words and bytes should be equal to each other
    
    let example32Bytes = [12, 34, 43, 45, 126, 251, 20, 93, 12, 35, 44, 55, 6, 25, 2, 99,
                        42, 64, 23, 4, 16, 201, 201, 43, 16, 38, 43, 9, 127, 231, 26, 73]
    
    let exampleJoinedBytes = "0c222b2d7efb145d0c232c37061902632a40170410c9c92b10262b097fe71a49"
    
    let exampleWords = ["around", "bacon", "slender", "worth", "rally", "company",
                        "correct", "grain", "damage", "cotton", "dog", "shock", "piano", "argue", "aware",
                        "bomb", "since", "flat", "another", "figure", "obey", "vicious", "hard", "calm"]

    override func setUp() {
        sc = SyncCrypto()
    }
    
    // MARK: - Unique serial bytes
    
    func testUniqueSerialBytes() {
        let count = 10
        XCTAssertEqual(SyncCrypto.uniqueSerialBytes(count: count)?.count, count)
    }
    
    func testUniqueSerialBytesOddNumberOfBytes() {
        let count = 3
        XCTAssertNil(SyncCrypto.uniqueSerialBytes(count: count))
    }
    
    // MARK: - Split bytes
    
    func testSplitBytes() {
        let result = sc.splitBytes(fromJoinedBytes: exampleJoinedBytes)
        XCTAssertEqual(result, example32Bytes)
    }
    
    func testSplitBytesEmptyString() {
        let result = sc.splitBytes(fromJoinedBytes: "")
        XCTAssertNil(result)
    }
    
    func testSplitBytesOddNumberOfBytes() {
        let result = sc.splitBytes(fromJoinedBytes: "0c2")
        XCTAssertNil(result)
    }
    
    func testSplitBytesNotHexString() {
        let result = sc.splitBytes(fromJoinedBytes: "Brave1")
        XCTAssertNil(result)
    }
    
    // MARK: - Join bytes
    
    // TODO: Add more edge case scenarios
    func testJoinBytes() {
        let joinedBytes = sc.joinBytes(fromCombinedBytes: example32Bytes)
        XCTAssertEqual(joinedBytes, exampleJoinedBytes)
    }
    
    func testJoinBytesNil() {
        let result = sc.joinBytes(fromCombinedBytes: nil)
        let emptyResult = ""
        XCTAssertEqual(result, emptyResult)
    }
    
    func testJoinBytesEmptyArray() {
        let result = sc.joinBytes(fromCombinedBytes: [])
        let emptyResult = ""
        XCTAssertEqual(result, emptyResult)
    }
    
    // MARK: - Passphrase

    func testPassphrase() {
        let result = sc.passphrase(fromBytes: example32Bytes)
        
        XCTAssert(result.isSuccess)
        XCTAssertEqual(result.value, exampleWords)
    }
    
    func testPassphraseEmptyArray() {
        let result = sc.passphrase(fromBytes: [])
        XCTAssert(result.isFailure)
    }
    
    func testPassphraseTooFewBytes() {
        let result = sc.passphrase(fromBytes: [1, 2, 3])
        XCTAssert(result.isFailure)
    }
    
    func testPassphraseTooManyBytes() {
        let tooManyBytes = example32Bytes + example32Bytes
        let result = sc.passphrase(fromBytes: tooManyBytes)
        XCTAssert(result.isFailure)
    }

    func testPassphraseNoUint8Array() {
        // at least one element has to be not convertible to UInt8
        let bytes = [-1, 999, 43, 45, 126, 251, 20, 93, 12, 35, 44, 55, 6, 25, 2, 99,
                     42, 64, 23, 4, 16, 201, 201, 43, 16, 38, 43, 9, 127, 231, 26, 73]
        
        let result = sc.passphrase(fromBytes: bytes)
        XCTAssert(result.isUnknownError)
    }
    
    // MARK: - toBytes
    
    func testToBytes() {
        let result = sc.bytes(fromPassphrase: exampleWords)
        XCTAssert(result.isSuccess)
        XCTAssertEqual(result.value, example32Bytes)
    }
    
    func testToBytesEmptyArray() {
        let result = sc.bytes(fromPassphrase: [])
        XCTAssert(result.isFailure)
    }
    
    func testToBytesToFewWords() {
        let result = sc.bytes(fromPassphrase: ["around", "bacon", "slender"])
        XCTAssert(result.isFailure)
    }
    
    func testToBytesTooManyWords() {
        let tooManyWords = exampleWords + exampleWords
        let result = sc.bytes(fromPassphrase: tooManyWords)
        XCTAssert(result.isFailure)
    }
    
    func testToBytesNotExistingWords() {
        let words = ["not_existing_word", "bacon", "slender", "worth", "rally", "company",
                     "correct", "grain", "damage", "cotton", "dog", "shock", "piano", "argue", "aware",
                     "bomb", "since", "flat", "another", "figure", "obey", "vicious", "hard", "calm"]
        
        let result = sc.bytes(fromPassphrase: words)
        XCTAssert(result.isFailure)
    }
    
    func testToBytesEmptyWord() {
        // first word-string is empty
        let words = ["", "bacon", "slender", "worth", "rally", "company",
                     "correct", "grain", "damage", "cotton", "dog", "shock", "piano", "argue", "aware",
                     "bomb", "since", "flat", "another", "figure", "obey", "vicious", "hard", "calm"]
        
        let result = sc.bytes(fromPassphrase: words)
        XCTAssert(result.isFailure)
    }

}

// MARK: - JSResult helper methods

fileprivate extension JSResult {
    var isSuccess: Bool {
        if case .success = self {
            return true
        }
        
        return false
    }
    
    var value: Value? {
        if case .success(let value) = self {
            return value
        }
        
        return nil
    }
    
    var isFailure: Bool {
        if case .failure = self {
            return true
        }
        
        return false
    }
    
    var isUnknownError: Bool {
        if case .failure(let error) = self {
            return "\(error)" == JSValue.unknownError
        }
        
        return false
    }
}

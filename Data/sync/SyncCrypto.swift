/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import WebKit
import Shared
import BraveShared
import SwiftyJSON
import JavaScriptCore

private let log = Logger.browserLogger

public enum JSResult<Value> {
    case success(Value)
    case failure(Error)
}

public class SyncCrypto {
    public init() {}
    
    private lazy var browserifyJS: BrowserifiedJS? = {
        return BrowserifiedJS(browserifiedScript: BraveCryptoJS())
    }()
    
    class func uniqueSerialBytes(count byteCount: Int) -> [Int]? {
        if byteCount % 2 != 0 { return nil }
        return (0..<byteCount).map { _ in Int(arc4random_uniform(256)) }
    }
    
    /// Takes a joined string of unique hex bytes (e.g. from QR code) and splits up the hex values
    /// fromJoinedBytes: a single string of hex data (even # of chars required): 897a6f0219fd2950
    /// return: integer values split into 8 bit groupings [0x98, 0x7a, 0x6f, ...]
    public func splitBytes(fromJoinedBytes bytes: String) -> [Int]? {
        var chars = bytes.map { String($0) }
        
        if chars.count % 2 == 1 {
            // Must be an even array
            return nil
        }
        
        var result = [Int]()
        while !chars.isEmpty {
            let hex = chars[0...1].reduce("", +)
            guard let integer = Int(hex, radix: 16) else {
                // bad error
                return nil
            }
            result.append(integer)
            chars.removeFirst(2) // According to docs this returns removed result, behavior is different (at least for Swift 2.3)
        }
        return result.isEmpty ? nil : result
    }
    
    /// Takes a string description of an array and returns a string of hex used for Sync
    /// fromCombinedBytes: ([123, 119, 25, 14, 85, 125])
    /// returns: 7b77190e557d
    public func joinBytes(fromCombinedBytes bytes: [Int]?) -> String {
        guard let bytes = bytes else {
            return ""
        }
        
        let hex = bytes.map { String($0, radix: 16, uppercase: false) }
        
        // Sync hex must be 2 chars, with optional leading 0
        let fullHex = hex.map { $0.count == 2 ? $0 : "0" + $0 }
        let combinedHex = fullHex.joined(separator: "")
        return combinedHex
    }
    
    // MARK: - brave-crypto javacsript methods
    
    /// Takes bytes values and returns associated English words
    /// returns: String of words from brave-crypto that map to those int values:
    /// "administrational experimental"
    public func passphrase(fromBytes bytes: [Int]) -> JSResult<[String]> {
        let bipWordLength = 24
        
        guard let browserifyJS = browserifyJS else { return .failure(JSValue.unknownError) }
        guard let bytesUInt8Array = browserifyJS.context.arrayToUint8Array(values: bytes) else {
            return .failure(JSValue.unknownError)
        }
        
        do {
            let result = try browserifyJS.call(
                functionName: BraveCryptoJS.Functions.fromBytesOrHex.rawValue, arguments: [bytesUInt8Array])
            
            let words = result.toString().components(separatedBy: " ")
            return words.count == bipWordLength ? .success(words) : .failure(JSValue.unknownError)
        } catch {
            log.error(error)
            return .failure(error)
        }
    }
    
    /// Takes English words and returns associated bytes.
    /// toBytes32: A string of words: "administrational experimental"
    /// returns array of integer values.
    public func bytes(fromPassphrase passphrase: [String]) -> JSResult<[Int]> {
        // TODO: Add some keyword validation

        let wordsWithSpaces = passphrase.joined(separator: " ")
        
        guard let browserifyJS = browserifyJS else {
            return .failure(JSValue.unknownError)
        }
        
        do {
            let result = try browserifyJS.call(functionName: BraveCryptoJS.Functions.toBytes32.rawValue,
                                                       arguments: [wordsWithSpaces])
            
            let bytes = result.toString().components(separatedBy: ",").compactMap { Int($0) }
            return bytes.count == 32 ? .success(bytes) : .failure(JSValue.unknownError)
        } catch {
            log.error(error)
            return .failure(error)
        }
    }
}

// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SDWebImage

extension FixedWidthInteger {
    /// The host-endian representation of this integer.
    ///
    /// If necessary, the byte order of this value is reversed from the typical
    /// byte order of this integer type.
    fileprivate var hostEndian: Self {
        let order = CFByteOrderGetCurrent()
        if order == CFByteOrderBigEndian.rawValue {
            return bigEndian
        }
        return littleEndian
    }
}

public class PrivateCDN {
    fileprivate static func payloadLength(for data: Data) -> UInt32? {
        guard data.count > 4 else {
            // Missing length field
            return nil
        }
        let length = data.withUnsafeBytes {
            return UInt32(bigEndian: $0.load(as: UInt32.self)).hostEndian
        }
        guard data.count > length else {
            // Payload shorter than expected length
            return nil
        }
        return length
    }
    
    /// Private CDN data unpadded to the original data
    ///
    /// The data which is obtained from the private CDN contains this structure:
    ///
    ///     |-----payload length-----|-----payload-----|-----padding-----|
    ///
    /// Where:
    ///
    ///  - `payload length`: a unsigned 32-bit integer referencing the length of `payload`
    ///  - `payload`: the raw unencoded binary variable-length payload
    ///  - `padding`: a variable-length string of uppercase `P` characters of
    ///               the length required to pad the file to its final size
    public static func unpadded(data: Data) -> Data? {
        guard let length = payloadLength(for: data) else { return nil }
        let startIndex = data.startIndex.advanced(by: 4)
        let endIndex = startIndex.advanced(by: Int(length))
        return data.subdata(in: startIndex..<endIndex)
    }
}

/// A custom SDWebImageCoder that will decode padded images that come from the Private CDN
public class PrivateCDNImageCoder: NSObject, SDWebImageCoder {
    public func canDecode(from data: Data?) -> Bool {
        guard let data = data else { return false }
        return PrivateCDN.payloadLength(for: data) != nil
    }
    public func decodedImage(with data: Data?) -> UIImage? {
        guard let paddedData = data, let unpaddedData = PrivateCDN.unpadded(data: paddedData) else {
            return nil
        }
        return SDWebImageCodersManager.sharedInstance().decodedImage(with: unpaddedData)
    }
    public func decompressedImage(with image: UIImage?, data: AutoreleasingUnsafeMutablePointer<NSData?>, options optionsDict: [String: NSObject]? = nil) -> UIImage? {
        image
    }
    public func canEncode(to format: SDImageFormat) -> Bool {
        return false
    }
    public func encodedData(with image: UIImage?, format: SDImageFormat) -> Data? {
        SDWebImageCodersManager.sharedInstance().encodedData(with: image, format: format)
    }
}

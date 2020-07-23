// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

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
        guard data.count > 4 else {
            // Missing length field
            return nil
        }
        let length = data.withUnsafeBytes {
            return $0.load(as: UInt32.self).hostEndian
        }
        guard data.count > length else {
            // Payload shorter than expected length
            return nil
        }
        
        let startIndex = data.startIndex.advanced(by: 4)
        let endIndex = startIndex.advanced(by: Int(length))
        return data.subdata(in: startIndex..<endIndex)
    }
}

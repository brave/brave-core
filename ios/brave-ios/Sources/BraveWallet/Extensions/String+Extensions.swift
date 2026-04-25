// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CryptoKit
import Foundation
import UIKit

extension String {
  private static let numberFormatterWithCurrentLocale = NumberFormatter().then {
    $0.numberStyle = .decimal
    $0.locale = Locale.current
  }

  private static let numberFormatterUsLocale = NumberFormatter().then {
    $0.numberStyle = .decimal
    $0.locale = .init(identifier: "en_US")
  }

  /// This will convert decimal string to use `en_US` locale decimal separator
  var normalizedDecimals: String {
    guard
      String.numberFormatterUsLocale.decimalSeparator
        != String.numberFormatterWithCurrentLocale.decimalSeparator
    else { return self }
    guard let number = String.numberFormatterWithCurrentLocale.number(from: self) else {
      return self
    }
    return String.numberFormatterUsLocale.string(from: number) ?? self
  }

  var hasUnknownUnicode: Bool {
    // same requirement as desktop. Valid: [0, 127]
    for c in unicodeScalars {
      let ci = Int(c.value)
      if ci > 127 {
        return true
      }
    }
    return false
  }

  var hasConsecutiveNewLines: Bool {
    // return true if string has two or more consecutive newline chars
    return range(of: "\\n{3,}", options: .regularExpression) != nil
  }

  var printableWithUnknownUnicode: String {
    var result = ""
    for c in unicodeScalars {
      let ci = Int(c.value)
      if let unicodeScalar = Unicode.Scalar(ci) {
        if ci == 10 {  // will keep newline char as it is
          result += "\n"
        } else {
          // ascii char will be displayed as it is
          // unknown (> 127) will be displayed as hex-encoded
          result += unicodeScalar.escaped(asASCII: true)
        }
      }
    }
    return result
  }

  var sha256: String {
    let data = Data(self.utf8)
    let hash = SHA256.hash(data: data)
    let hashString = hash.map { String(format: "%02hhx", $0) }.joined()
    return hashString
  }

  var qrCodeImage: UIImage? {
    guard let data = self.data(using: .utf8) else { return nil }
    let context = CIContext()
    let filter = CIFilter.qrCodeGenerator()
    filter.message = data
    filter.correctionLevel = "H"
    if let image = filter.outputImage,
      let cgImage = context.createCGImage(image, from: image.extent)
    {
      return UIImage(cgImage: cgImage)
    }
    return nil
  }
}

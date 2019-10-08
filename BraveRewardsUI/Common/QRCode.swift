/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import CoreImage

final class QRCode: NSObject {
  static func image(for code: String, size: CGSize) -> UIImage? {
    guard let filter = CIFilter(name: "CIQRCodeGenerator"),
      let data = code.data(using: .utf8) else {
        return nil
    }
    filter.setValue(data, forKey: "inputMessage")
    guard let ciImage = filter.outputImage else {
      return nil
    }
    let genSize = ciImage.extent.integral.size
    let scale = UIScreen.main.scale
    let scaledSize = size.applying(CGAffineTransform(scaleX: scale, y: scale))
    let resizedImage = ciImage.transformed(by: CGAffineTransform(
      scaleX: scaledSize.width / genSize.width,
      y: scaledSize.height / genSize.height
    ))
    return UIImage(
      ciImage: resizedImage,
      scale: scale,
      orientation: .up
    )
  }
}

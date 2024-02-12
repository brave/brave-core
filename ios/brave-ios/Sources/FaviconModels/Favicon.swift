// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import Shared

/// Structure representing a Favicon
public class Favicon: Codable {
  public let image: UIImage?
  public let isMonogramImage: Bool
  public let backgroundColor: UIColor

  public static let defaultImage = UIImage(named: "defaultFavicon",
                                           in: .module,
                                           compatibleWith: nil)!

  public static let `default` = Favicon(image: Favicon.defaultImage,
                                        isMonogramImage: false,
                                        backgroundColor: .clear)
  
  public var hasTransparentBackground: Bool {
    backgroundColor.rgba == UIColor.clear.rgba
  }

  public init(image: UIImage?, isMonogramImage: Bool, backgroundColor: UIColor) {
    self.image = image
    self.isMonogramImage = isMonogramImage
    self.backgroundColor = backgroundColor
  }

  public required init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    if let data = try container.decodeIfPresent(Data.self, forKey: .image) {
      let scale = try container.decodeIfPresent(CGFloat.self, forKey: .imageScale) ?? 1.0
      image = UIImage(data: data, scale: scale)
    } else {
      image = nil
    }

    isMonogramImage = try container.decode(Bool.self, forKey: .isMonogramImage)
    backgroundColor = try UIColor(rgba: container.decode(UInt32.self, forKey: .backgroundColor))
  }

  public func encode(to encoder: Encoder) throws {
    var container = encoder.container(keyedBy: CodingKeys.self)
    try container.encode(image?.pngData(), forKey: .image)
    try container.encode(image?.scale, forKey: .imageScale)
    try container.encode(isMonogramImage, forKey: .isMonogramImage)
    try container.encode(backgroundColor.rgba, forKey: .backgroundColor)
  }
  
  private enum CodingKeys: CodingKey {
    case image
    case imageScale
    case isMonogramImage
    case backgroundColor
  }
}

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

/// This is for conformance only, but in the future we might add custom NTP theming that doesn't change background images.
protocol NTPThemeable {}

/// Backwards compatible schema to support both wallpapers and campaigns.
struct NTPSchema: Codable, NTPThemeable {
  /// Sponsor schema version.
  var schemaVersion: Int
  /// Wallpapers to show on new tab page.
  var wallpapers: [NTPWallpaper]?
  /// Brand logo to show on new tab page. Can be overridden by wallpaper logo.
  var logo: NTPLogo?
  /// Campaigns to show on the new tab page.
  var campaigns: [NTPCampaign]?
}

/// Sponsors that can be shown on the new tab page.
struct NTPSponsor: Codable, NTPThemeable {
  /// Sponsor schema version.
  var schemaVersion: Int
  /// Campaigns to show on the new tab page.
  var campaigns: [NTPCampaign]
}

/// Campaigns that can be shown on the new tab page.
struct NTPCampaign: Codable {
  /// Wallpapers to show on new tab page.
  var wallpapers: [NTPWallpaper]
  /// Brand logo to show on new tab page. Can be overridden by wallpaper logo.
  var logo: NTPLogo?
}

/// Wallpaper that can be shown on the new tab page.
/// A class instead of a struct since it includes a mutating property (`image`). If a struct this will lead to any `willSet` / `didSet`
///     observers being called again, since the struct instance will have been mutated.
class NTPWallpaper: Codable {
  let imageUrl: String

  /// Only available for sponsored images, not normal wallpapers. Overrides default campaign logo
  let logo: NTPLogo?

  /// Required instead of `CGPoint` due to x/y being optionals
  let focalPoint: FocalPoint?

  // Only available for sponsored images, not normal wallpapers
  let creativeInstanceId: String?

  /// Only available for normal wallpapers, not for sponsored images
  let credit: Credit?

  /// Whether the wallpaper is a packaged resource or a remote one, impacts how it should be loaded
  let packaged: Bool?

  init(imageUrl: String, logo: NTPLogo?, focalPoint: FocalPoint?, creativeInstanceId: String?) {
    self.imageUrl = imageUrl
    self.logo = logo
    self.focalPoint = focalPoint
    self.creativeInstanceId = creativeInstanceId
    self.credit = nil
    self.packaged = nil
  }

  struct Credit: Codable {
    let name: String
    let url: String?
  }

  struct FocalPoint: Codable {
    let x: CGFloat?
    let y: CGFloat?
  }

  var image: UIImage? {
    // Remote resources are downloaded files, so must be loaded differently
    if packaged == true {
      // Load without cache if possible
      if let path = Bundle.module.path(forResource: imageUrl, ofType: nil) {
        return UIImage(contentsOfFile: path)
      }

      // Load with cache
      return UIImage(named: imageUrl)
    }
    return UIImage(contentsOfFile: imageUrl)
  }
}

/// Brand logo that can be shown on the new tab page.
class NTPLogo: Codable {
  let imageUrl: String
  let alt: String
  let companyName: String
  /// Url to take the user to after the logo is tapped.
  let destinationUrl: String

  init(imageUrl: String, alt: String, companyName: String, destinationUrl: String) {
    self.imageUrl = imageUrl
    self.alt = alt
    self.companyName = companyName
    self.destinationUrl = destinationUrl
  }

  var image: UIImage? {
    UIImage(contentsOfFile: imageUrl)
  }
}

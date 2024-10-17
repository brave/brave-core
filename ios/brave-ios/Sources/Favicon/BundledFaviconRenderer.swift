// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import FaviconModels
import Foundation
import Shared
import UIKit
import os.log

/// A class for rendering a Bundled FavIcon onto a `UIImage`
public actor BundledFaviconRenderer {

  /// Folder where custom favicons are stored.
  static let faviconOverridesDirectory = "favorite_overrides"
  /// For each favicon override, there should be a file that contains info of what background color to use.
  static let faviconOverridesBackgroundSuffix = ".background_color"

  @MainActor
  public static func loadIcon(url: URL) async throws -> Favicon {
    var icon = await customIcon(for: url)
    if icon == nil {
      icon = await bundledIcon(for: url)
    }
    guard let icon else {
      // No need to render a monogram
      throw FaviconError.noBundledImages
    }

    // Render the Favicon on a UIImage
    return await UIImage.renderFavicon(
      icon.image,
      backgroundColor: icon.backgroundColor,
      shouldScale: false
    )
  }

  // MARK: - Custom Icons

  /// Icon attributes for any custom icon overrides
  ///
  /// If the app does not contain a custom icon for the site provided `nil`
  /// will be returned
  private static func customIcon(for url: URL) async -> (image: UIImage, backgroundColor: UIColor)?
  {
    guard
      let folder = try? await AsyncFileManager.default.url(
        for: .applicationSupportDirectory,
        appending: Self.faviconOverridesDirectory,
        create: true
      )
    else {
      return nil
    }

    let fileName = url.absoluteString.toBase64()
    let backgroundName = fileName + Self.faviconOverridesBackgroundSuffix
    let backgroundPath = folder.appendingPathComponent(backgroundName)
    do {
      let colorString = try String(contentsOf: backgroundPath)
      let colorFromHex = UIColor(colorString: colorString)

      if await AsyncFileManager.default.fileExists(
        atPath: folder.appendingPathComponent(fileName).path
      ) {
        let imagePath = folder.appendingPathComponent(fileName)
        if let image = UIImage(contentsOfFile: imagePath.path) {
          return (image, colorFromHex)
        }
        return nil
      }
    } catch {
      return nil
    }
    return nil
  }

  // MARK: - Bundled Icons

  private static var loadBundledIconsTask: Task<Void, Never>?
  private static func loadBundledIcons() async {
    if let loadBundledIconsTask {
      await loadBundledIconsTask.value
      return
    }
    let loadBundledIconsTask = Task {
      guard let filePath = Bundle.module.path(forResource: "top_sites", ofType: "json") else {
        Logger.module.error("Failed to get bundle path for \"top_sites.json\"")
        return
      }
      do {
        let file = try Data(contentsOf: URL(fileURLWithPath: filePath))
        let json = try JSONDecoder().decode([TopSite].self, from: file)
        var icons: [String: (color: UIColor, url: String)] = [:]
        json.forEach({
          guard let url = $0.domain,
            let color = $0.backgroundColor?.lowercased(),
            let path = $0.imageURL?.replacingOccurrences(of: ".png", with: "")
          else {
            return
          }
          let filePath = Bundle.module.path(forResource: "TopSites/" + path, ofType: "png")
          if let filePath = filePath {
            if color == "#fff" {
              icons[url] = (.white, filePath)
            } else {
              icons[url] = (
                UIColor(colorString: color.replacingOccurrences(of: "#", with: "")), filePath
              )
            }
          }
        })
        bundledIcons = icons
      } catch {
        Logger.module.error(
          "Failed to get default icons at \(filePath): \(error.localizedDescription)"
        )
      }
    }
    Self.loadBundledIconsTask = loadBundledIconsTask
    await loadBundledIconsTask.value
  }
  /// Icon attributes for icons that are bundled in the app by default.
  ///
  /// If the app does not contain the icon for the site provided `nil` will be
  /// returned
  private static func bundledIcon(for url: URL) async -> (image: UIImage, backgroundColor: UIColor)?
  {
    if bundledIcons.isEmpty {
      await loadBundledIcons()
    }
    // Problem: Sites like amazon exist with .ca/.de and many other tlds.
    // Solution: They are stored in the default icons list as "amazon" instead of "amazon.com" this allows us to have favicons for every tld."
    // Here, If the site is in the multiRegionDomain array look it up via its second level domain (amazon) instead of its baseDomain (amazon.com)
    let hostName = url.hostSLD
    var bundleIcon: (color: UIColor, url: String)?
    if Self.multiRegionDomains.contains(hostName), let icon = Self.bundledIcons[hostName] {
      bundleIcon = icon
    } else if let name = url.baseDomain, let icon = Self.bundledIcons[name] {
      bundleIcon = icon
    }
    guard let icon = bundleIcon,
      let image = UIImage(contentsOfFile: icon.url),
      let scaledImage = image.createScaled(CGSize(width: 40.0, height: 40.0))
    else {
      return nil
    }

    return (scaledImage, icon.color)
  }

  private static let multiRegionDomains = ["craigslist", "google", "amazon"]

  private static var bundledIcons: [String: (color: UIColor, url: String)] = [:]

  private struct TopSite: Codable {
    let domain: String?
    let backgroundColor: String?
    let imageURL: String?

    private enum CodingKeys: String, CodingKey {
      case domain
      case backgroundColor = "background_color"
      case imageURL = "image_url"
    }
  }
}

extension Favicon {
  @MainActor
  public static func renderImage(
    _ image: UIImage,
    backgroundColor: UIColor?,
    shouldScale: Bool
  ) async -> Favicon {
    await UIImage.renderFavicon(image, backgroundColor: backgroundColor, shouldScale: shouldScale)
  }
}

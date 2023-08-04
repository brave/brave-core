// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import Preferences
import BraveCore
import os.log

enum NTPWallpaper {
  case image(NTPBackgroundImage)
  case sponsoredImage(NTPSponsoredImageBackground)
  case superReferral(NTPSponsoredImageBackground, code: String)
  
  var backgroundImage: UIImage? {
    let imagePath: URL
    switch self {
    case .image(let background):
      imagePath = background.imagePath
    case .sponsoredImage(let background):
      imagePath = background.imagePath
    case .superReferral(let background, _):
      imagePath = background.imagePath
    }
    return UIImage(contentsOfFile: imagePath.path)
  }
  
  var logoImage: UIImage? {
    let imagePath: URL?
    switch self {
    case .image:
      imagePath = nil
    case .sponsoredImage(let background):
      imagePath = background.logo.imagePath
    case .superReferral(let background, _):
      imagePath = background.logo.imagePath
    }
    return imagePath.flatMap { UIImage(contentsOfFile: $0.path) }
  }
  
  var focalPoint: CGPoint? {
    switch self {
    case .image:
      return nil // Will eventually return a real value
    case .sponsoredImage(let background):
      return background.focalPoint
    case .superReferral(let background, _):
      return background.focalPoint
    }
  }
}

public class NTPDataSource {
  
  private(set) var privateBrowsingManager: PrivateBrowsingManager

  var initializeFavorites: ((_ sites: [NTPSponsoredImageTopSite]?) -> Void)?

  /// Custom homepage spec requirement:
  /// If we fail to fetch super referrer, and it succeeds at later time,
  /// default favorites are going to be replaced with the ones from the super referrer.
  /// This happens only if the user has not changed default favorites.
  var replaceFavoritesIfNeeded: ((_ sites: [NTPSponsoredImageTopSite]?) -> Void)?

  // Data is static to avoid duplicate loads

  /// This is the number of backgrounds that must appear before a background can be repeated.
  /// So if background `3` is shown, it cannot be shown until this many backgrounds are shown, then `3` can be shown again.
  /// This does not apply to sponsored images.
  /// This is reset on each launch, so `3` can be shown again if app is removed from memory.
  /// This number _must_ be less than the number of backgrounds!
  private static let numberOfDuplicateAvoidance = 6
  /// The number of images in a sponsorship rotation.
  ///     i.e. This number will be repeated before a new sponsorship is shown.
  ///          If sposnored image is shown as Nth image, this number of normal images will be shown before Nth is reached again.
  private static let sponsorshipShowRate = 4
  /// On this value, a sponsored image will be shown.
  private static let sponsorshipShowValue = 2

  /// The counter that indicates what background should be shown, this is used to determine when a new
  ///     sponsored image should be shown. (`1` means, first image in cycle N, should be shown).
  /// One example, if rotation is every 4 images, but sponsored image should be shown as 2nd image, then this will
  ///     be reset back to `1` after reaching `4`, and when the value is `2`, a sponsored image will be shown.
  /// This can be easily converted to a preference to persist
  private var backgroundRotationCounter = 1

  let service: NTPBackgroundImagesService

  public init(service: NTPBackgroundImagesService, privateBrowsingManager: PrivateBrowsingManager) {
    self.service = service
    self.privateBrowsingManager = privateBrowsingManager
    
    Preferences.NewTabPage.selectedCustomTheme.observe(from: self)
    
    self.service.sponsoredImageDataUpdated = { [weak self] _ in
      self?.sponsorComponentUpdated()
    }
  }
  
  deinit {
    self.service.sponsoredImageDataUpdated = nil
  }

  // This is used to prevent the same handful of backgrounds from being shown.
  //  It will track the last N pictures that have been shown and prevent them from being shown
  //  until they are 'old' and dropped from this array.
  // Currently only supports normal backgrounds, as sponsored images are not supposed to be duplicate.
  // This can 'easily' be adjusted to support both sets by switching to String, and using filePath to identify uniqueness.
  private var lastBackgroundChoices = [Int]()

  private enum ImageRotationStrategy {
    /// Special strategy for sponsored images, uses in-memory property to keep track of which image to show.
    case sponsoredRotation
    /// Uses random images, keeps in-memory list of recently viewed images to avoid showing it too often.
    case randomOrderAvoidDuplicates
  }

  func newBackground() -> NTPWallpaper? {
    if !Preferences.NewTabPage.backgroundImages.value { return nil }

    // Identifying the background array to use
    let (backgroundSet, strategy) = {
      () -> ([NTPWallpaper], ImageRotationStrategy) in

      if let theme = service.superReferralImageData,
         case let refCode = service.superReferralCode,
         !refCode.isEmpty,
        Preferences.NewTabPage.selectedCustomTheme.value != nil {
        return (theme.campaigns.flatMap(\.backgrounds).map { NTPWallpaper.superReferral($0, code: refCode) }, .randomOrderAvoidDuplicates)
      }

      if let sponsor = service.sponsoredImageData {
        let attemptSponsored =
          Preferences.NewTabPage.backgroundSponsoredImages.value
          && backgroundRotationCounter == NTPDataSource.sponsorshipShowValue
          && !privateBrowsingManager.isPrivateBrowsing

        if attemptSponsored {
          // Pick the campaign randomly
          let campaignIndex: Int = Int.random(in: 0..<sponsor.campaigns.count)

          if let campaign = sponsor.campaigns[safe: campaignIndex] {
            return (campaign.backgrounds.map(NTPWallpaper.sponsoredImage), .sponsoredRotation)
          }
        }
      }

      if service.backgroundImages.isEmpty {
        return ([NTPWallpaper.image(.fallback)], .randomOrderAvoidDuplicates)
      }
      return (service.backgroundImages.map(NTPWallpaper.image), .randomOrderAvoidDuplicates)
    }()

    if backgroundSet.isEmpty { return nil }

    // Choosing the actual index / item to use
    let backgroundIndex = { () -> Int in
      switch strategy {
      case .sponsoredRotation:
        return Int.random(in: 0..<backgroundSet.count)

      case .randomOrderAvoidDuplicates:
        let availableRange = 0..<backgroundSet.count
        // This takes all indeces and filters out ones that were shown recently
        let availableBackgroundIndeces = availableRange.filter {
          !self.lastBackgroundChoices.contains($0)
        }
        // Due to how many display modes currently exist, the background avoidance counter may get utilized on a smaller subset.
        // This can be repro by swapping between normal backgrounds and a super referrer, where all available indeces get squeezed out, resulting in an empty set.
        // To avoid issues, first fallback results in full set.

        // Chooses a new random index to use from the available indeces
        let chosenIndex = availableBackgroundIndeces.randomElement() ?? availableRange.randomElement() ?? -1
        assert(chosenIndex >= 0, "NTP index was nil, this is terrible.")
        assert(chosenIndex < backgroundSet.count, "NTP index is too large, BAD!")

        // This index is now added to 'past' tracking list to prevent duplicates
        self.lastBackgroundChoices.append(chosenIndex)
        // Trimming to fixed length to release older backgrounds

        self.lastBackgroundChoices = self.lastBackgroundChoices
          .suffix(min(backgroundSet.count - 1, NTPDataSource.numberOfDuplicateAvoidance))
        return chosenIndex
      }
    }()

    // Force back to `0` if at end
    backgroundRotationCounter %= NTPDataSource.sponsorshipShowRate
    // Increment regardless, this is a counter, not an index, so smallest should be `1`
    backgroundRotationCounter += 1

    guard let bgWithIndex = backgroundSet[safe: backgroundIndex] else { return nil }
    return bgWithIndex
  }
  
  func sponsorComponentUpdated() {
    if let superReferralImageData = service.superReferralImageData, superReferralImageData.isSuperReferral {
      if Preferences.NewTabPage.preloadedFavoritiesInitialized.value {
        replaceFavoritesIfNeeded?(superReferralImageData.topSites)
      } else {
        initializeFavorites?(superReferralImageData.topSites)
      }
    } else {
      // Force to set up basic favorites if it hasn't been done already.
      initializeFavorites?(nil)
    }
  }
}

extension NTPDataSource: PreferencesObserver {
  public func preferencesDidChange(for key: String) {
    let customThemePref = Preferences.NewTabPage.selectedCustomTheme
    let installedThemesPref = Preferences.NewTabPage.installedCustomThemes

    switch key {
    case customThemePref.key:
      let installedThemes = installedThemesPref.value
      if let theme = customThemePref.value, !installedThemes.contains(theme) {
        installedThemesPref.value = installedThemesPref.value + [theme]
      }
    default:
      break
    }
  }
}

extension NTPSponsoredImageTopSite {
  var asFavoriteSite: FavoriteSite? {
    guard let url = destinationURL else {
      return nil
    }
    return FavoriteSite(url, name)
  }
}

extension NTPBackgroundImage {
  static let fallback: NTPBackgroundImage = .init(
    imagePath: Bundle.module.url(forResource: "corwin-prescott-3", withExtension: "jpg")!,
    author: "Corwin Prescott",
    link: URL(string: "https://www.brave.com")!
  )
}

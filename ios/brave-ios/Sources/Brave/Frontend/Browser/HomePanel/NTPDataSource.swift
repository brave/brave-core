// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import Shared
import UIKit
import os.log

enum NTPWallpaper {
  case image(NTPBackgroundImage)
  case sponsoredMedia(NTPSponsoredImageBackground)
  case superReferral(NTPSponsoredImageBackground, code: String)

  var backgroundVideoPath: URL? {
    if case .sponsoredMedia(let background) = self {
      return background.isVideoFile ? background.imagePath : nil
    }
    return nil
  }

  var backgroundImage: UIImage? {
    let imagePath: URL
    switch self {
    case .image(let background):
      imagePath = background.imagePath
    case .sponsoredMedia(let background):
      if background.isVideoFile {
        return nil
      }
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
    case .sponsoredMedia(let background):
      imagePath = background.logo.imagePath
    case .superReferral(let background, _):
      imagePath = background.logo.imagePath
    }
    return imagePath.flatMap { UIImage(contentsOfFile: $0.path) }
  }

  var focalPoint: CGPoint? {
    switch self {
    case .image:
      return nil  // Will eventually return a real value
    case .sponsoredMedia(let background):
      return background.focalPoint
    case .superReferral(let background, _):
      return background.focalPoint
    }
  }
}

public class NTPDataSource {
  private var rewards: BraveRewards?

  private(set) var privateBrowsingManager: PrivateBrowsingManager

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

  let service: NTPBackgroundImagesService

  public init(
    service: NTPBackgroundImagesService,
    rewards: BraveRewards?,
    privateBrowsingManager: PrivateBrowsingManager
  ) {
    self.service = service
    self.rewards = rewards
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

  func shouldAttemptSponsoredMedia() -> Bool {
    return
      Preferences.NewTabPage.backgroundMediaType.isSponsored
      && Preferences.NewTabPage.backgroundRotationCounter.value
        == service.initialCountToBrandedWallpaper
      && !privateBrowsingManager.isPrivateBrowsing
  }

  func getSponsoredMediaBackground() -> NTPWallpaper? {
    guard let sponsoredImageData = service.sponsoredImageData
    else { return nil }

    guard let newTabPageAd = rewards?.ads.maybeGetPrefetchedNewTabPageAd()
    else { return nil }

    let gracePeriodEnded = hasGracePeriodEnded(sponsoredImageData)

    let isSponsoredVideoAllowed =
      Preferences.NewTabPage.backgroundMediaType == .sponsoredImagesAndVideos

    for campaign in sponsoredImageData.campaigns {
      if campaign.campaignId != newTabPageAd.campaignID {
        continue
      }

      for creative in campaign.backgrounds {
        // Campaigns with disabled metrics are always eligible to be shown.
        let isEligible =
          (gracePeriodEnded || creative.metricType == .disabled) && creative.logo.imagePath != nil
          && creative.creativeInstanceId == newTabPageAd.creativeInstanceID
          && (!creative.isVideoFile || isSponsoredVideoAllowed)

        if isEligible {
          return .sponsoredMedia(creative)
        }
      }
    }

    rewards?.ads.onFailedToPrefetchNewTabPageAd(
      placementId: newTabPageAd.placementID,
      creativeInstanceId: newTabPageAd.creativeInstanceID
    )
    return nil
  }

  func getImageBackground() -> NTPWallpaper? {
    // Identifying the background array to use
    let backgroundSet = {
      () -> [NTPWallpaper] in

      if let theme = service.superReferralImageData,
        case let refCode = service.superReferralCode,
        !refCode.isEmpty,
        Preferences.NewTabPage.selectedCustomTheme.value != nil
      {
        return
          theme.campaigns.flatMap(\.backgrounds).map {
            NTPWallpaper.superReferral($0, code: refCode)
          }
      }

      if service.backgroundImages.isEmpty {
        return [NTPWallpaper.image(.fallback)]
      }
      return service.backgroundImages.map(NTPWallpaper.image)
    }()

    if backgroundSet.isEmpty { return nil }

    // Choosing the actual index / item to use
    let backgroundIndex = { () -> Int in
      let availableRange = 0..<backgroundSet.count
      // This takes all indeces and filters out ones that were shown recently
      let availableBackgroundIndeces = availableRange.filter {
        !self.lastBackgroundChoices.contains($0)
      }
      // Due to how many display modes currently exist, the background avoidance counter may get utilized on a smaller subset.
      // This can be repro by swapping between normal backgrounds and a super referrer, where all available indeces get squeezed out, resulting in an empty set.
      // To avoid issues, first fallback results in full set.

      // Chooses a new random index to use from the available indeces
      let chosenIndex =
        availableBackgroundIndeces.randomElement() ?? availableRange.randomElement() ?? -1
      assert(chosenIndex >= 0, "NTP index was nil, this is terrible.")
      assert(chosenIndex < backgroundSet.count, "NTP index is too large, BAD!")

      // This index is now added to 'past' tracking list to prevent duplicates
      self.lastBackgroundChoices.append(chosenIndex)
      // Trimming to fixed length to release older backgrounds

      self.lastBackgroundChoices = self.lastBackgroundChoices
        .suffix(min(backgroundSet.count - 1, NTPDataSource.numberOfDuplicateAvoidance))
      return chosenIndex
    }()

    return backgroundSet[safe: backgroundIndex]
  }

  func newBackground() -> NTPWallpaper? {
    if !Preferences.NewTabPage.backgroundImages.value { return nil }

    var background: NTPWallpaper? = nil

    if shouldAttemptSponsoredMedia() {
      background = getSponsoredMediaBackground()
    }

    if background == nil {
      background = getImageBackground()
    }

    // Force back to `0` if at end
    Preferences.NewTabPage.backgroundRotationCounter.value %= service.countToBrandedWallpaper
    // Increment regardless, this is a counter, not an index, so smallest should be `1`
    Preferences.NewTabPage.backgroundRotationCounter.value += 1

    return background
  }

  func sponsorComponentUpdated() {
    if let superReferralImageData = service.superReferralImageData,
      superReferralImageData.isSuperReferral,
      Preferences.NewTabPage.preloadedFavoritiesInitialized.value
    {
      replaceFavoritesIfNeeded?(superReferralImageData.topSites)
    }
  }

  private func hasGracePeriodEnded(_ sponsoredImageData: NTPSponsoredImageData) -> Bool {
    if BraveRewards.Configuration.current().isDebug == true {
      // If debug mode is enabled, consider it ended.
      return true
    }

    guard let gracePeriod = sponsoredImageData.gracePeriod?.doubleValue,
      let installDate = Preferences.P3A.installationDate.value
    else {
      // If either the grace period or install date is not set, consider it ended.
      return true
    }

    let gracePeriodEndAt = installDate + TimeInterval(gracePeriod)
    if Date.now >= gracePeriodEndAt {
      return true
    }

    Logger.module.info(
      "Sponsored images not shown: Grace period after installation is still active until \(gracePeriodEndAt)"
    )
    return false
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

extension NTPSponsoredImageBackground {
  var isVideoFile: Bool {
    imagePath.pathExtension == "mp4"
  }
}

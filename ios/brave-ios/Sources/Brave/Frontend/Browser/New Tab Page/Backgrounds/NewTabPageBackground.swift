// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Foundation
import Growth
import Preferences
import UIKit

/// The current background for a given New Tab Page.
///
/// This class is responsable for providing the background image for a new tab
/// page, and altering said background based on changes from outside of the NTP
/// such as the user changing Private Mode or disabling the background images
/// prefs while the user is currently viewing a New Tab Page.
class NewTabPageBackground: PreferencesObserver {
  /// The source of new tab page backgrounds
  private let dataSource: NTPDataSource
  private let rewards: BraveRewards
  /// The current background image & possibly sponsor
  private(set) var currentBackground: NTPWallpaper? {
    didSet {
      wallpaperId = UUID()
      changed?()
    }
  }
  /// A unique wallpaper identifier
  private(set) var wallpaperId = UUID()
  /// The background/wallpaper image if available
  var backgroundImage: UIImage? {
    currentBackground?.backgroundImage
  }
  /// The background video URL if available
  var backgroundVideoPath: URL? {
    currentBackground?.backgroundVideoPath
  }
  /// The sponsors logo if available
  var sponsorLogoImage: UIImage? {
    currentBackground?.logoImage
  }
  /// A block called when the current background image/sponsored logo changes
  /// while the New Tab Page is active
  var changed: (() -> Void)?
  /// Create a background holder given a source of all NTP background images
  init(dataSource: NTPDataSource, rewards: BraveRewards) {
    self.dataSource = dataSource
    self.rewards = rewards
    self.currentBackground = dataSource.newBackground()

    Preferences.NewTabPage.backgroundImages.observe(from: self)
    Preferences.NewTabPage.backgroundMediaTypeRaw.observe(from: self)
    Preferences.NewTabPage.selectedCustomTheme.observe(from: self)

    recordSponsoredMediaTypeP3A()
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  private var timer: Timer?

  func preferencesDidChange(for key: String) {
    // Debounce multiple changes to preferences, since toggling bg images
    // cause sponsored images to also be toggled at the same time
    timer?.invalidate()
    timer = Timer.scheduledTimer(
      withTimeInterval: 0.25,
      repeats: false,
      block: { [weak self] _ in
        guard let self = self else { return }
        self.currentBackground = self.dataSource.newBackground()
        self.recordSponsoredMediaTypeP3A()
      }
    )
  }

  private func recordSponsoredMediaTypeP3A() {
    // Question: What type of new tab page sponsored media is shown?
    enum Answer: Int, CaseIterable {
      case disabled = 0
      case images = 1
      case imagesAndVideos = 2
    }

    var answer = Answer.disabled
    if Preferences.NewTabPage.backgroundImages.value
      && Preferences.NewTabPage.backgroundMediaType.isSponsored
    {
      answer =
        Preferences.NewTabPage.backgroundMediaType == .sponsoredImagesAndVideos
          && rewards.ads.shouldShowSponsoredImagesAndVideosSetting()
        ? .imagesAndVideos
        : .images
    }

    UmaHistogramEnumeration("Brave.NTP.SponsoredMediaType", sample: answer)
  }
}

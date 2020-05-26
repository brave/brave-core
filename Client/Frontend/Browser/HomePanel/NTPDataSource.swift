// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveRewards

class NTPDataSource {
    
    var initializeFavorites: ((_ sites: [CustomTheme.TopSite]?) -> Void)?
    
    /// Custom homepage spec requirement:
    /// If we fail to fetch super referrer, and it succeeds at later time,
    /// default favorites are going to be replaced with the ones from the super referrer.
    /// This happens only if the user has not changed default favorites.
    var replaceFavoritesIfNeeded: ((_ sites: [CustomTheme.TopSite]?) -> Void)?

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
    // This can be easily converted to a preference to persist
    private var backgroundRotationCounter = 1
    
    /// The `index` for the next sponsored image to show.
    ///     e.g. if `0` it should show the `0th` sponsored image _when a sponsored image should be shown_
    /// This does _not_ impact _when_ a sponsored image is shown, but rather _what_ sponsored image is shown.
    /// The maximum of this value is undetermined, as that depends on how many images exist in the current sponsorship.
    // This can be easily converted to a preference to persist
    private var sponsoredBackgroundRotationIndex = 0

    private lazy var downloader = NTPDownloader()
    private var customTheme: CustomTheme?
    private var sponsor: NTPSponsor?
    
    private lazy var standardBackgrounds: [NTPBackground] = {
        let backgroundFilePath = "ntp-data"
        guard let backgroundData = self.loadData(file: backgroundFilePath) else { return [] }
        
        do {
            return try JSONDecoder().decode([NTPBackground].self, from: backgroundData)
        } catch {
            return []
        }
    }()
    
    init() {
        downloader.delegate = self
        
        Preferences.NewTabPage.backgroundSponsoredImages.observe(from: self)
        Preferences.NewTabPage.selectedCustomTheme.observe(from: self)
    }
    
    func startFetching() {
        let downloadType = downloader.currentResourceType
        // For super referrer we want to load assets from cache first, then check if new resources
        // are on the server.
        // This is because as super referrer install we never want to show other images than the ones
        // provided by the super referrer.
        // In the future we might want to extend this preload from cache logic to other resource types.
        if case .superReferral = downloadType {
            downloader.preloadCustomTheme()
        }
        
        if downloader.delegate != nil {
            downloader.notifyObservers(for: downloader.currentResourceType)
        }
    }
    
    func fetchSpecificResource(_ type: NTPDownloader.ResourceType) {
        if downloader.delegate != nil {
            downloader.notifyObservers(for: type)
        }
    }
    
    // This is used to prevent the same handful of backgrounds from being shown.
    //  It will track the last N pictures that have been shown and prevent them from being shown
    //  until they are 'old' and dropped from this array.
    // Currently only supports normal backgrounds, as sponsored images are not supposed to be duplicate.
    // This can 'easily' be adjusted to support both sets by switching to String, and using filePath to identify uniqueness.
    private var lastBackgroundChoices = [Int]()
    
    enum BackgroundType {
        case regular
        case withBrandLogo(_ logo: NTPLogo?)
        case withQRCode(_ code: String)
    }
    
    private enum ImageRotationStrategy {
        /// Special strategy for sponsored images, uses in-memory property to keep track of which image to show.
        case sponsoredRotation
        /// Uses random images, keeps in-memory list of recently viewed images to avoid showing it too often.
        case randomOrderAvoidDuplicates
    }
    
    func newBackground() -> (NTPBackground, BackgroundType)? {
        if !Preferences.NewTabPage.backgroundImages.value { return nil }
        
        // Identifying the background array to use
        let (backgroundSet, backgroundType, strategy) = {
            () -> ([NTPBackground], BackgroundType, ImageRotationStrategy) in
            
            if let theme = customTheme,
                let refCode = theme.refCode,
                Preferences.NewTabPage.selectedCustomTheme.value != nil {
                return (theme.wallpapers, .withQRCode(refCode), .randomOrderAvoidDuplicates)
            }
            
            if let sponsor = sponsor {
                let attemptSponsored = Preferences.NewTabPage.backgroundSponsoredImages.value
                    && backgroundRotationCounter == NTPDataSource.sponsorshipShowValue
                    && !PrivateBrowsingManager.shared.isPrivateBrowsing
                
                if attemptSponsored {
                    return (sponsor.wallpapers, .withBrandLogo(sponsor.logo), .sponsoredRotation)
                }
            }
            
            return (standardBackgrounds, .regular, .randomOrderAvoidDuplicates)
        }()
        
        if backgroundSet.isEmpty { return nil }
        
        // Choosing the actual index / item to use
        let backgroundIndex = { () -> Int in
            switch strategy {
            case .sponsoredRotation:
                defer {
                    sponsoredBackgroundRotationIndex += 1
                    // Force a max, and wrap back down if it is hit.
                    sponsoredBackgroundRotationIndex %= backgroundSet.count
                }
                return sponsoredBackgroundRotationIndex
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
        return (bgWithIndex, backgroundType)
    }
    
    private func loadData(file: String) -> Data? {
        guard let filePath = Bundle.main.path(forResource: file, ofType: "json") else {
            return nil
        }
        
        do {
            let backgroundData = try Data(contentsOf: URL(fileURLWithPath: filePath))
            return backgroundData
        } catch {
            Logger.browserLogger.error("Failed to get bundle path for \(file)")
        }
        
        return nil
    }
}

extension NTPDataSource: NTPDownloaderDelegate {
    func onSponsorUpdated(sponsor: NTPSponsor?) {
        self.sponsor = sponsor
        // Force to set up basic favorites if it hasn't been done already.
        initializeFavorites?(nil)
    }
    
    func onThemeUpdated(theme: CustomTheme?) {
        self.customTheme = theme
        
        if Preferences.NewTabPage.preloadedFavoritiesInitialized.value {
            replaceFavoritesIfNeeded?(theme?.topSites)
        } else {
            initializeFavorites?(theme?.topSites)
        }
    }
    
    func preloadCustomTheme(theme: CustomTheme?) {
        self.customTheme = theme
    }
}

extension NTPDataSource: PreferencesObserver {
    func preferencesDidChange(for key: String) {
        let sponsoredPref = Preferences.NewTabPage.backgroundSponsoredImages
        let customThemePref = Preferences.NewTabPage.selectedCustomTheme
        let installedThemesPref = Preferences.NewTabPage.installedCustomThemes
        
        switch key {
        case sponsoredPref.key:
            if sponsoredPref.value {
                // Issue download only when no custom theme is currently set.
                if customThemePref.value == nil {
                    Preferences.NTP.ntpCheckDate.value = nil
                    downloader.delegate = self
                    fetchSpecificResource(.sponsor)
                }

            } else {
                downloader.delegate = nil
                do {
                    try downloader.removeCampaign(type: .sponsor)
                } catch {
                    Logger.browserLogger.error(error)
                }
            }
        case customThemePref.key:
            downloader.delegate = self
            
            let installedThemes = installedThemesPref.value
            if let theme = customThemePref.value, !installedThemes.contains(theme) {
                installedThemesPref.value = installedThemesPref.value + [theme]
            }
        default:
            break
        }
    }
}

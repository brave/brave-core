// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveRewards

class NTPBackgroundDataSource {
    
    struct Background: Codable {
        let imageUrl: String
        
        /// Required instead of `CGPoint` due to x/y being optionals
        let focalPoint: FocalPoint?
        
        /// Only available for normal wallpapers, not for sponsored images
        let credit: Credit?
        
        /// Whether the background is a packaged resource or a remote one, impacts how it should be loaded
        let packaged: Bool?
        
        init(imageUrl: String, focalPoint: FocalPoint?) {
            self.imageUrl = imageUrl
            self.focalPoint = focalPoint
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
        
        lazy var image: UIImage? = {
            // Remote resources are downloaded files, so must be loaded differently
            packaged == true ? UIImage(named: imageUrl) : UIImage(contentsOfFile: imageUrl)
        }()
    }
    
    struct Sponsor: Codable {
        let wallpapers: [Background]
        var logo: Logo
        
        struct Logo: Codable {
            let imageUrl: String
            let alt: String
            let companyName: String
            let destinationUrl: String
            
            lazy var image: UIImage? = {
                UIImage(contentsOfFile: imageUrl)
            }()
        }
    }

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
    private var sponsor: Sponsor?
    
    private lazy var standardBackgrounds: [Background] = {
        let backgroundFilePath = "ntp-data"
        guard let backgroundData = self.loadData(file: backgroundFilePath) else { return [] }
        
        do {
            return try JSONDecoder().decode([Background].self, from: backgroundData)
        } catch {
            return []
        }
    }()
    
    init() {
        if Preferences.NewTabPage.backgroundSponsoredImages.value {
            downloader.delegate = self
        }
        
        Preferences.NewTabPage.backgroundSponsoredImages.observe(from: self)
    }
    
    // This is used to prevent the same handful of backgrounds from being shown.
    //  It will track the last N pictures that have been shown and prevent them from being shown
    //  until they are 'old' and dropped from this array.
    // Currently only supports normal backgrounds, as sponsored images are not supposed to be duplicate.
    // This can 'easily' be adjusted to support both sets by switching to String, and using filePath to identify uniqueness.
    private var lastBackgroundChoices = [Int]()
    
    func newBackground() -> (Background, Sponsor?)? {
        if !Preferences.NewTabPage.backgroundImages.value { return nil }
        
        // Identifying the background array to use
        let (backgroundSet, useSponsor) = { () -> ([NTPBackgroundDataSource.Background], Bool) in
            // Determine what type of background to display
            let attemptSponsored = Preferences.NewTabPage.backgroundSponsoredImages.value
                && backgroundRotationCounter == NTPBackgroundDataSource.sponsorshipShowValue
                && !PrivateBrowsingManager.shared.isPrivateBrowsing
            
            // Sponsor is lazy-loaded so only want to access it if needed.
            if attemptSponsored, let sponsoredWallpapers = sponsor?.wallpapers {
                return (sponsoredWallpapers, true)
            }
            return (standardBackgrounds, false)
        }()
        
        if backgroundSet.isEmpty { return nil }
        
        // Choosing the actual index / item to use
        let backgroundIndex = { () -> Int in
            if useSponsor {
                defer {
                    sponsoredBackgroundRotationIndex += 1
                    // Force a max, and wrap back down if it is hit.
                    sponsoredBackgroundRotationIndex %= backgroundSet.count
                }
                return sponsoredBackgroundRotationIndex
            }
            
            let availableRange = 0..<backgroundSet.count
            // This takes all indeces and filters out ones that were shown recently
            let availableBackgroundIndeces = availableRange.filter {
                !self.lastBackgroundChoices.contains($0)
            }
            // Chooses a new random index to use from the available indeces
            // -1 will result in a `nil` return
            let chosenIndex = availableBackgroundIndeces.randomElement() ?? -1
            assert(chosenIndex >= 0, "NTP index was nil, this is terrible.")
            assert(chosenIndex < backgroundSet.count, "NTP index is too large, BAD!")

            // This index is now added to 'past' tracking list to prevent duplicates
            self.lastBackgroundChoices.append(chosenIndex)
            // Trimming to fixed length to release older backgrounds
            self.lastBackgroundChoices = self.lastBackgroundChoices.suffix(NTPBackgroundDataSource.numberOfDuplicateAvoidance)
            return chosenIndex
        }()
        
        // Force back to `0` if at end
        backgroundRotationCounter %= NTPBackgroundDataSource.sponsorshipShowRate
        // Increment regardless, this is a counter, not an index, so smallest should be `1`
        backgroundRotationCounter += 1
        
        // Item is returned based on our special index.
        
        return (backgroundSet[backgroundIndex], useSponsor ? sponsor : nil)
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

extension NTPBackgroundDataSource: NTPDownloaderDelegate {
    func onNTPUpdated(ntpInfo: NTPBackgroundDataSource.Sponsor?) {
        sponsor = ntpInfo
    }
}

extension NTPBackgroundDataSource: PreferencesObserver {
    func preferencesDidChange(for key: String) {
        let sponsoredPref = Preferences.NewTabPage.backgroundSponsoredImages
        if sponsoredPref.key == key {
            if sponsoredPref.value {
                // Enabled? -> issue download
                downloader.delegate = self

            } else {
                downloader.delegate = nil
                do {
                    try downloader.removeCampaign()
                } catch {
                    Logger.browserLogger.error(error)
                }
            }
        }
    }
}

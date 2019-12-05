// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared

class BackgroundImage {
    
    struct Background {
        let imageFileName: String
        let center: CGFloat
        let isSponsored: Bool
        let credit: (name: String, url: String?)?
        
        lazy var image: UIImage? = {
            return UIImage(named: imageFileName)
        }()
    }

    // Data is static to avoid duplicate loads
    
    /// This is the number of backgrounds that must appear before a background can be repeated.
    /// So if background `3` is shown, it cannot be shown until this many backgrounds are shown, then `3` can be shown again.
    /// This does not apply to sponsored images.
    /// This is reset on each launch, so `3` can be shown again if app is removed from memory.
    /// This number _must_ be less than the number of backgrounds!
    private static let numberOfDuplicateAvoidance = 6
    private static let sponsorshipShowRate = 4 // e.g. 4 == 25% or "every 4th image"
    
    let info: Background?
    static var hasSponsor: Bool { sponsors?.count ?? 0 > 0 }
    private static var sponsors: [Background]?
    private static var standardBackgrounds: [Background]?
    
    // This is used to prevent the same handful of backgrounds from being shown.
    //  It will track the last N pictures that have been shown and prevent them from being shown
    //  until they are 'old' and dropped from this array.
    // Currently only supports normal backgrounds, as sponsored images are not supposed to be duplicate.
    // This can 'easily' be adjusted to support both sets by switching to String, and using filePath to identify uniqueness.
    private static var lastBackgroundChoices = [Int]()
    
    init(sponsoredFilePath: String = "ntp-sponsored", backgroundFilePath: String = "ntp-data") {
        
        if !Preferences.NewTabPage.backgroundImages.value {
            // Do absolutely nothing
            self.info = nil
            return
        }
        
        // Setting up normal backgrounds
        if BackgroundImage.standardBackgrounds == nil {
            BackgroundImage.standardBackgrounds = BackgroundImage.generateStandardData(file: backgroundFilePath)
        }
        
        // Setting up sponsored backgrounds
        if BackgroundImage.sponsors == nil && Preferences.NewTabPage.backgroundSponsoredImages.value {
            BackgroundImage.sponsors = BackgroundImage.generateSponsoredData(file: sponsoredFilePath)
        }
        
        self.info = BackgroundImage.randomBackground()
    }
    
    private static func generateSponsoredData(file: String) -> [Background] {
        guard let json = BackgroundImage.loadImageJSON(file: file),
            let region = NSLocale.current.regionCode else {
                return []
        }
        
        let dateFormatter = DateFormatter().then {
            $0.locale = Locale(identifier: "en_US_POSIX")
            $0.dateFormat = "yyyy-MM-dd'T'HH:mm:ssZZZZZ"
            $0.calendar = Calendar(identifier: .gregorian)
        }
        
        let today = Date()
        // Filter down to only regional supported items
        let regionals = json.filter { ($0["regions"] as? [String])?.contains(region) == true }
        
        // Filter down to sponsors that fit the date requirements
        let live = regionals.filter { item in
            guard let dates = item["dates"] as? [String: String],
                let start = dateFormatter.date(from: dates["start"] ?? ""),
                let end = dateFormatter.date(from: dates["end"] ?? "") else {
                    return false
            }
            
            return today > start && today < end
        }
        
        return generateBackgroundData(data: live, isSponsored: true)
    }
    
    private static func generateStandardData(file: String) -> [Background] {
        guard let json = BackgroundImage.loadImageJSON(file: file) else { return [] }
        return generateBackgroundData(data: json, isSponsored: false)
    }
    
    private static func generateBackgroundData(data: [[String: Any]], isSponsored: Bool) -> [Background] {
        return data.compactMap { item in
            guard let image = item["image"] as? String,
                let center = item["center"] as? CGFloat else { return nil }
            
            if let credit = item["credit"] as? [String: String],
                let name = credit["name"] {
                return Background(imageFileName: image, center: center, isSponsored: isSponsored, credit: (name, credit["url"]))
            }
            
            return Background(imageFileName: image, center: center, isSponsored: isSponsored, credit: nil)
        }
    }
    
    private static func randomBackground() -> Background? {
        // Determine what type of background to display
        let useSponsor = Preferences.NewTabPage.backgroundSponsoredImages.value
            && hasSponsor
            && Int.random(in: 0..<sponsorshipShowRate) == 0
        guard let dataSet = useSponsor ? sponsors : standardBackgrounds else { return nil }
        if dataSet.isEmpty { return nil }
        
        let availableRange = 0..<dataSet.count
        var randomBackgroundIndex = Int.random(in: availableRange)
        if !useSponsor {
            /// This takes all indeces and filters out ones that were shown recently
            let availableBackgroundIndeces = availableRange.filter {
                !lastBackgroundChoices.contains($0)
            }
            // Chooses a new random index to use from the available indeces
            // -1 will result in a `nil` return
            randomBackgroundIndex = availableBackgroundIndeces.randomElement() ?? -1
            assert(randomBackgroundIndex >= 0, "randomBackgroundIndex was nil, this is terrible.")
            
            // This index is now added to 'past' tracking list to prevent duplicates
            lastBackgroundChoices.append(randomBackgroundIndex)
            // Trimming to fixed length to release older backgrounds
            lastBackgroundChoices = lastBackgroundChoices.suffix(numberOfDuplicateAvoidance)
        }
        
        // Item is returned based on our random index.
        // Could generally use `randomElement()`, but for non-sponsored images, certain indeces are ignored.
        return dataSet[safe: randomBackgroundIndex]
    }
    
    private static func loadImageJSON(file: String) -> [[String: Any]]? {
        guard let filePath = Bundle.main.path(forResource: file, ofType: "json") else {
            return nil
        }
        
        do {
            let fileData = try Data(contentsOf: URL(fileURLWithPath: filePath))
            let json = try JSONSerialization.jsonObject(with: fileData, options: []) as? [[String: Any]]
            return json
        } catch {
            Logger.browserLogger.error("Failed to get bundle path for \(file)")
        }
        
        return nil
    }
    
    /// This is a temp work around to address the weird settings requirements for the initial sponsored immage
    ///     support in these super specific regions, as it is also to ignore if a sponsored item is actually available.
    static var showSponsoredSetting: Bool {
        let regions = ["JP", "CN", "KR", "KP", "TW", "MN", "MO", "GU", "MP", "SG", "IN", "PK", "MV", "BD", "BT", "NP", "UZ"]
        return regions.contains(NSLocale.current.regionCode ?? "")
    }
}

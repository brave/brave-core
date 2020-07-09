/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared

private let log = Logger.browserLogger

typealias FavoriteSite = (url: URL, title: String)

struct PreloadedFavorites {
    /// Returns a list of websites that should be preloaded for specific region. Currently all users get the same websites.
    static func getList() -> [FavoriteSite] {
        func appendPopularEnglishWebsites() -> [FavoriteSite] {
            var list = [FavoriteSite]()
            
            if let url = URL(string: "https://m.youtube.com") {
                list.append(FavoriteSite(url: url, title: "Youtube"))
            }
            
            if let url = URL(string: "https://www.amazon.com/") {
                list.append(FavoriteSite(url: url, title: "Amazon"))
            }
            
            if let url = URL(string: "https://www.wikipedia.org/") {
                list.append(FavoriteSite(url: url, title: "Wikipedia"))
            }
            
            if let url = URL(string: "https://mobile.twitter.com/") {
                list.append(FavoriteSite(url: url, title: "Twitter"))
            }
            
            if let url = URL(string: "https://reddit.com/") {
                list.append(FavoriteSite(url: url, title: "Reddit"))
            }
            
            if let url = URL(string: "https://brave.com/msupport/") {
                list.append(FavoriteSite(url: url, title: Strings.NTP.braveSupportFavoriteTitle))
            }

            return list
        }

        var preloadedFavorites = [FavoriteSite]()

        // Locale consists of language and region, region makes more sense when it comes to setting preloaded websites imo.
        let region = Locale.current.regionCode ?? "" // Empty string will go to the default switch case
        log.debug("Preloading favorites, current region: \(region)")

        switch region {
        case "PL":
            // We don't do any per region preloaded favorites at the moment.
            // But if we would like to, it is as easy as adding a region switch case and adding websites to the list.

            // try? list.append(FavoriteSite(url: "https://allegro.pl/".asURL(), title: "Allegro"))
            preloadedFavorites += appendPopularEnglishWebsites()
            break
        default:
            preloadedFavorites += appendPopularEnglishWebsites()
        }

        return preloadedFavorites
    }
}

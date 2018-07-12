/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

private let log = Logger.browserLogger

typealias FavoriteSite = (url: URL, title: String)

struct PreloadedFavorites {
    /// Returns a list of websites that should be preloaded for specific region. Currently all users get the same websites.
    static func getList() -> [FavoriteSite] {
        func appendPopularEnglishWebsites() -> [FavoriteSite] {
            var list = [FavoriteSite]()
            try? list.append(FavoriteSite(url: "https://m.youtube.com".asURL(), title: "Youtube"))
            try? list.append(FavoriteSite(url: "https://www.amazon.com/".asURL(), title: "Amazon"))
            try? list.append(FavoriteSite(url: "https://www.wikipedia.org/".asURL(), title: "Wikipedia"))
            try? list.append(FavoriteSite(url: "https://mobile.twitter.com/".asURL(), title: "Twitter"))
            try? list.append(FavoriteSite(url: "https://reddit.com/".asURL(), title: "Reddit"))
            try? list.append(FavoriteSite(url: "https://coinmarketcap.com/".asURL(), title: "CoinMarketCap"))

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

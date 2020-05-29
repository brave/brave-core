// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A set of attributes that are used to customized the Brave app.
struct CustomTheme: Codable, NTPBackgroundProtocol, NTPThemeable {
    private enum CodingKeys: String, CodingKey {
        case wallpapers, logo, topSites, themeName
    }
    
    /// Name of the custom theme.
    var themeName: String
    /// Wallpapers to show on new tab page.
    let wallpapers: [NTPBackground]
    /// Brand's logo to show on new tab page.
    var logo: NTPLogo?
    /// Custom favorites that are created at first launch.
    var topSites: [TopSite]?
    /// Optional: Referral code attached to the theme.
    var refCode: String?
    
    struct TopSite: Codable {
        private enum CodingKeys: String, CodingKey {
            case name, destinationUrl, iconUrl, backgroundColor
        }
        
        let name: String
        let destinationUrl: String
        let iconUrl: String
        let backgroundColor: String
        
        var asFavoriteSite: FavoriteSite? {
            guard let url = URL(string: destinationUrl) else {
                assertionFailure("Could not cast \(destinationUrl) to URL")
                return nil
            }
            return FavoriteSite(url, name)
        }
    }
}

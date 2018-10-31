/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import XCGLogger

/// Accessors to find what a new tab should do when created without a URL.
struct NewTabAccessors {
    static let Default = NewTabPage.topSites

    static func getNewTabPage() -> NewTabPage {
        return Default
    }
}

/// Enum to encode what should happen when the user opens a new tab without a URL.
enum NewTabPage: String {
    case blankPage = "Blank"
    case homePage = "HomePage"
    case topSites = "TopSites"

    var settingTitle: String {
        switch self {
        case .blankPage:
            return Strings.SettingsNewTabBlankPage
        case .homePage:
            return Strings.SettingsNewTabHomePage
        case .topSites:
            return Strings.SettingsNewTabTopSites
        }
    }

    var url: URL? {
        // TODO: #258: If we aren't going to inherit the NewPageTab preference (since we'll always open to favourites),
        //       probably best to remove `NewTabPage` all-together in the future and refactor out its usages.
        
        // For now, we are going to just default to 0 which used to be `HomePanelType.topSites`
        return URL(string: "#panel=0", relativeTo: UIConstants.AboutHomePage as URL)!
    }

    static let allValues = [blankPage, topSites, homePage]
}

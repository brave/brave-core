// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import BraveUI

extension AppDelegate {
    /// Setup basic control defaults based on Brave design system colors
    ///
    /// Only set values here that should be universally accepted as a default color for said
    /// control. Do not apply appearance overrides here to solve for laziness of not wanting to set
    /// a color multiple times.
    ///
    /// - warning: Be careful adjusting colors here, and make sure impact is well known
    func applyAppearanceDefaults() {
        // important! for privacy concerns, otherwise UI can bleed through
        UIToolbar.appearance().do {
            $0.tintColor = .braveOrange
            $0.standardAppearance = {
                let appearance = UIToolbarAppearance()
                appearance.configureWithDefaultBackground()
                appearance.backgroundColor = .braveBackground
                return appearance
            }()
        }
        
        UINavigationBar.appearance().do {
            $0.tintColor = .braveOrange
            $0.standardAppearance = {
                let appearance = UINavigationBarAppearance()
                appearance.configureWithDefaultBackground()
                appearance.titleTextAttributes = [.foregroundColor: UIColor.braveLabel]
                appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.braveLabel]
                appearance.backgroundColor = .braveBackground
                return appearance
            }()
        }
        
        UISwitch.appearance().onTintColor = UIColor.braveOrange
        
        /// Used as color a table will use as the base (e.g. background)
        let tablePrimaryColor = UIColor.braveGroupedBackground
        /// Used to augment `tablePrimaryColor` above
        let tableSecondaryColor = UIColor.secondaryBraveGroupedBackground
        
        UITableView.appearance().backgroundColor = tablePrimaryColor
        UITableView.appearance().separatorColor = .braveSeparator
        
        UITableViewCell.appearance().do {
            $0.tintColor = .braveOrange
            $0.backgroundColor = tableSecondaryColor
        }
        
        UIImageView.appearance(whenContainedInInstancesOf: [SettingsViewController.self])
            .tintColor = .braveLabel

        UIView.appearance(whenContainedInInstancesOf: [UITableViewHeaderFooterView.self])
            .backgroundColor = tablePrimaryColor
        
        UILabel.appearance(whenContainedInInstancesOf: [UITableView.self]).textColor = .braveLabel
        UILabel.appearance(whenContainedInInstancesOf: [UICollectionReusableView.self])
            .textColor = .braveLabel
        
        UITextField.appearance().textColor = .braveLabel
    }
}

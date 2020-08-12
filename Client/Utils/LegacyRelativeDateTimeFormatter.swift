// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A replacement for a subset of the features available in `RelativeDateTimeFormatter` but usable in iOS 12
///
/// Uses `ThirdParty/DateTools/DateTools.bundle` for localizations
///
/// Can be removed when deployment target is raised to iOS 13
class LegacyRelativeDateTimeFormatter {
    
    /// Obtain a localized string that looks like "1 day ago", "2 weeks ago", etc. based on the given date and
    /// reference date.
    func localizedString(for date: Date, relativeTo referenceDate: Date) -> String? {
        guard let dateToolsBundlePath = Bundle.main.resourceURL?.appendingPathComponent("DateTools.bundle").path,
            let dateToolsBundle = Bundle(path: dateToolsBundlePath) else {
                return nil
        }
        
        func localizedStringFromDateToolsBundle(_ key: String, _ args: CVarArg...) -> String {
            return String(
                format: NSLocalizedString(key, tableName: "DateTools", bundle: dateToolsBundle, value: "", comment: ""),
                arguments: args
            )
        }
        
        func localizedStringForComponentValue(_ value: Int, formats: (plural: String, single: String)) -> String {
            if value > 1 {
                return localizedStringFromDateToolsBundle(formats.plural, value)
            }
            return localizedStringFromDateToolsBundle(formats.single)
        }
        
        let components = Calendar.current.dateComponents(
            [.year, .month, .weekOfYear, .day, .hour, .minute, .second],
            from: date,
            to: referenceDate
        )
        if let year = components.year, year > 0 {
            return localizedStringForComponentValue(year, formats: ("%d years ago", "1 year ago"))
        }
        if let month = components.month, month > 0 {
            return localizedStringForComponentValue(month, formats: ("%d months ago", "1 month ago"))
        }
        if let week = components.weekOfYear, week > 0 {
            return localizedStringForComponentValue(week, formats: ("%d weeks ago", "1 week ago"))
        }
        if let day = components.day, day > 0 {
            return localizedStringForComponentValue(day, formats: ("%d days ago", "1 day ago"))
        }
        if let hour = components.hour, hour > 0 {
            return localizedStringForComponentValue(hour, formats: ("%d hours ago", "1 hour ago"))
        }
        if let minute = components.minute, minute > 0 {
            return localizedStringForComponentValue(minute, formats: ("%d minutes ago", "1 minute ago"))
        }
        if let second = components.second, second > 0 {
            return localizedStringForComponentValue(second, formats: ("%d seconds ago", "1 second ago"))
        }
        return nil
    }
}

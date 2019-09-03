// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

public struct AppReview {
    static let firstThreshold = 14
    static let secondThreshold = 41
    static let lastThreshold = 121
    static let minimumDaysBetweenReviewRequest = 60
    
    public static func shouldRequestReview(date: Date = Date()) -> Bool {
        let launchCount = Preferences.Review.launchCount.value
        let threshold = Preferences.Review.threshold.value
        
        var daysSinceLastRequest = 0
        if let previousRequest = Preferences.Review.lastReviewDate.value {
            daysSinceLastRequest = Calendar.current.dateComponents([.day], from: previousRequest, to: date).day ?? 0
        } else {
            daysSinceLastRequest = minimumDaysBetweenReviewRequest
        }
        
        if launchCount <= threshold || daysSinceLastRequest < minimumDaysBetweenReviewRequest {
            return false
        }
        
        Preferences.Review.lastReviewDate.value = date
        
        switch threshold {
        case firstThreshold:
            Preferences.Review.threshold.value = secondThreshold
        case secondThreshold:
            Preferences.Review.threshold.value = lastThreshold
        default:
            break
        }
        
        return true
    }
}

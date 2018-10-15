/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import XCGLogger

private let log = Logger.browserLogger

public class DAU {

    /// Default installation date for legacy woi version.
    public static let defaultWoiDate = "2016-01-04"
    
    private static let apiVersion = 1
    private static let baseUrl = "https://laptop-updates.brave.com/\(apiVersion)/usage/ios?platform=ios"
    
    /// Number of seconds that determins when a user is "active"
    private let activeUserDuration = 10.0
    
    private var launchTimer: Timer?
    private let today: Date
    private var todayComponents: DateComponents {
        return (Calendar.current as NSCalendar).components([.day, .month, .year, .weekday], from: today)
    }
    
    public init(date: Date = Date()) {
        today = date
    }
    
    public func sendPingToServer() {
        if launchTimer != nil { return }
        launchTimer =
            Timer.scheduledTimer(
                timeInterval: activeUserDuration,
                target: self,
                selector: #selector(sendPingToServerInternal),
                userInfo: nil,
                repeats: false)
    }
    
    @objc public func sendPingToServerInternal() {
        guard let params = paramsAndPrefsSetup() else {
            log.debug("dau, no changes detected, no server ping")
            return
        }
        
        // Sending ping to server
        var pingRequest = URLComponents(string: DAU.baseUrl)
        pingRequest?.queryItems = params
        
        guard let pingRequestUrl = pingRequest?.url else {
            log.error("Stats failed to update, via invalud URL: \(pingRequest?.description ?? "😡")")
            return
        }
        
        log.debug("send ping to server, url: \(pingRequestUrl)")
        
        let task = URLSession.shared.dataTask(with: pingRequestUrl) { _, _, error in
            if let e = error {
                log.error("status update error: \(e)")
                return
            }
            
            // Successful, we have certainly validated first ping
            Preferences.DAU.firstPingSuccess.value = true
        }
        
        task.resume()
    }
    
    /// Return params query or nil if no ping should be send to server.
    func paramsAndPrefsSetup() -> [URLQueryItem]? {
        var params = [channelParam(), versionParam()]
        
        /// This is not the same as `firstLaunch` concept, due to DAU delay, this may var be `true` on a subsequent launch, if server ping failed
        let firstPing = Preferences.DAU.firstPingSuccess.value
        
        // All installs prior to this key existing (e.g. intallWeek == unknown) were set to `defaultWoiDate`
        // Enough time has passed where accounting for installs prior to this DAU improvement is unnecessary
        
        // See `woi` logic elsewhere to see fallback is handled
        
        // This could lead to an upgraded device having no `woi`, and that's fine
        if firstPing {
            Preferences.DAU.weekOfInstallation.value = todayComponents.weeksMonday
        }
        
        guard let dauStatParams = dauStatParams(firstPing: firstPing) else {
            log.debug("dau, no changes detected, no server ping")
            return nil
        }
        
        params += dauStatParams
        params += [
            firstLaunchParam(for: firstPing),
            // Must be after setting up the preferences
            weekOfInstallationParam()
        ]

        if let referralCode = UserReferralProgram.getReferralCode() {
            params.append(URLQueryItem(name: "ref", value: referralCode))
            UrpLog.log("DAU ping with added ref, params: \(params)")
        }
        
        let secsMonthYear = [Int(today.timeIntervalSince1970), todayComponents.month, todayComponents.year]
        Preferences.DAU.lastLaunchInfo.value = secsMonthYear
        
        return params
    }
    
    func channelParam(for channel: AppBuildChannel = AppConstants.BuildChannel) -> URLQueryItem {
        return URLQueryItem(name: "channel", value: channel.isRelease ? "stable" : "beta")
    }
    
    func versionParam(for version: String = AppInfo.appVersion) -> URLQueryItem {
        var version = version
        if DAU.shouldAppend0(toVersion: version) {
            version += ".0"
        }
        return URLQueryItem(name: "version", value: version)
    }

    /// All app versions for dau pings must be saved in x.x.x format where x are digits.
    static func shouldAppend0(toVersion version: String) -> Bool {
        let correctAppVersionPattern = "^\\d+.\\d+$"
        do {
            let regex = try NSRegularExpression(pattern: correctAppVersionPattern, options: [])
            let match = regex.firstMatch(in: version, options: [], range: NSRange(location: 0, length: version.count))
            
            return match != nil
        } catch {
            log.error("Version regex pattern error")
            return false
        }
    }
    
    func firstLaunchParam(for isFirst: Bool) -> URLQueryItem {
        return URLQueryItem(name: "first", value: isFirst.description)
    }
    
    /// All first app installs are normalized to first day of the week.
    /// e.g. user installs app on wednesday 2017-22-11, his install date is recorded as of 2017-20-11(Monday)
    func weekOfInstallationParam(for woi: String? = Preferences.DAU.weekOfInstallation.value) -> URLQueryItem {
        var woi = woi
        // This _should_ be set all the time
        if woi == nil {
            woi = DAU.defaultWoiDate
            log.error("woi, is nil, using default: \(woi ?? "")")
        }
        return URLQueryItem(name: "woi", value: woi)
    }
    
    /// Returns nil if no dau changes detected.
    func dauStatParams(
        _ dauStat: [Int?]? = Preferences.DAU.lastLaunchInfo.value,
        firstPing: Bool,
        channel: AppBuildChannel = AppConstants.BuildChannel) -> [URLQueryItem]? {
        
        func dauParams(_ daily: Bool, _ weekly: Bool, _ monthly: Bool) -> [URLQueryItem] {
            return ["daily": daily, "weekly": weekly, "monthly": monthly].map {
                URLQueryItem(name: $0.key, value: $0.value.description)
            }
        }
        
        if firstPing || channel == .developer {
            return dauParams(true, true, true)
        }
        
        let month = todayComponents.month
        let year = todayComponents.year
        
        guard let stat = dauStat?.compactMap({ $0 }) else {
            log.error("Cannot cast dauStat to [Int]")
            return nil
        }
        
        guard stat.count == 3 else {
            log.error("dauStat array must contain exactly 3 elements")
            return nil
        }
        
        let dSecs = Int(today.timeIntervalSince1970) - stat[0]
        let _month = stat[1]
        let _year = stat[2]
        let SECONDS_IN_A_DAY = 86400
        let SECONDS_IN_A_WEEK = 7 * 86400
        
        // On first ping, the user is all three of these
        let daily = dSecs >= SECONDS_IN_A_DAY
        let weekly = dSecs >= SECONDS_IN_A_WEEK
        let monthly = month != _month || year != _year
        log.debug("Dau stat params, daily: \(daily), weekly: \(weekly), monthly:\(monthly), dSecs: \(dSecs)")
        if !daily && !weekly && !monthly {
            // No changes, no ping
            return nil
        }
        
        return dauParams(daily, weekly, monthly)
    }
}

private extension DateComponents {
    /// Returns date of current week's monday in YYYY-MM-DD format
    var weeksMonday: String {
        var isSunday: Bool {
            guard let weekday = weekday else {
                log.error("Weekday is nil")
                return false
            }
            return weekday == 1
        }
        
        // Make sure all required date components are set.
        guard let _ = day, let _ = month, let _ = year, let weekday = weekday else {
            log.error("Date components are missing")
            return ""
        }
        
        guard let today = Calendar.current.date(from: self) else {
            log.error("Cannot create date from date components")
            return ""
        }
        
        let dayInSeconds = 60 * 60 * 24
        // Sunday is first weekday so we need to handle this day differently, can't just substract it.
        let sundayToMondayDayDifference = 6
        let dayDifference = isSunday ? sundayToMondayDayDifference : weekday - 2 // -2 because monday is second weekday
        
        let monday = Date(timeInterval: -TimeInterval(dayDifference * dayInSeconds), since: today)
        let mondayComponents = (Calendar.current as NSCalendar).components([.day, .month, .year], from: monday)
        
        guard let mYear = mondayComponents.year, let mMonth = mondayComponents.month, let mDay = mondayComponents.day else {
            log.error("First monday of the week components are nil")
            return ""
        }
        
        return "\(mYear)-\(mMonth)-\(mDay)"
    }
}

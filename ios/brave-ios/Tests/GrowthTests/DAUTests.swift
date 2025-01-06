// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import Shared
import XCTest

@testable import Growth

extension DAU {
  fileprivate convenience init() {
    self.init(braveCoreStats: nil)
  }
}

class DAUTests: XCTestCase {

  override func setUp() {
    super.setUp()

    Preferences.DAU.weekOfInstallation.reset()
    Preferences.DAU.lastLaunchInfo.reset()
    Preferences.DAU.firstPingParam.reset()
    Preferences.DAU.installationDate.reset()
  }

  // 7-7-07 at 12noon GMT
  let date = Date(timeIntervalSince1970: 1_183_809_600)
  let dau = DAU()

  func testChannelParam() {
    let releaseExpected = URLQueryItem(name: "channel", value: "release")
    XCTAssertEqual(dau.channelParam(for: .release), releaseExpected)

    let externalBetaExpected = URLQueryItem(name: "channel", value: "beta")
    XCTAssertEqual(dau.channelParam(for: .beta), externalBetaExpected)

    let nightlyExpected = URLQueryItem(name: "channel", value: "nightly")
    XCTAssertEqual(dau.channelParam(for: .nightly), nightlyExpected)

    let debugExpected = URLQueryItem(name: "channel", value: "nightly")
    XCTAssertEqual(dau.channelParam(for: .debug), debugExpected)
  }

  func testVersionParam() {
    var expected = URLQueryItem(name: "version", value: "1.1.0")
    XCTAssertEqual(dau.versionParam(for: "1.1"), expected)

    expected = URLQueryItem(name: "version", value: "1.1.1")
    XCTAssertEqual(dau.versionParam(for: "1.1.1"), expected)
  }

  func testShouldAppend0() {
    XCTAssertFalse(DAU.shouldAppend0(toVersion: "1.5.2"))
    XCTAssertFalse(DAU.shouldAppend0(toVersion: "1.52.2"))
    XCTAssertFalse(DAU.shouldAppend0(toVersion: "11.5.23"))
    XCTAssertFalse(DAU.shouldAppend0(toVersion: "11.55.23"))

    XCTAssertTrue(DAU.shouldAppend0(toVersion: "1.5"))
    XCTAssertTrue(DAU.shouldAppend0(toVersion: "11.5"))
    XCTAssertTrue(DAU.shouldAppend0(toVersion: "1.10"))
  }

  func testFirstLaunchParam() {
    var expected: URLQueryItem!
    expected = URLQueryItem(name: "first", value: "true")
    XCTAssertEqual(dau.firstLaunchParam(for: true), expected)

    expected = URLQueryItem(name: "first", value: "false")
    XCTAssertEqual(dau.firstLaunchParam(for: false), expected)
  }

  func testWOIParam() {
    var expected: URLQueryItem!
    expected = URLQueryItem(name: "woi", value: DAU.defaultWoiDate)
    XCTAssertEqual(dau.weekOfInstallationParam(for: nil), expected)

    expected = URLQueryItem(name: "woi", value: "absolutelyNothing")
    XCTAssertEqual(dau.weekOfInstallationParam(for: "absolutelyNothing"), expected)

    expected = URLQueryItem(name: "woi", value: "2012-12-12")
    XCTAssertEqual(dau.weekOfInstallationParam(for: "2012-12-12"), expected)
  }

  func testDtoiParamNewUser() {
    let dateString = "2017-11-10"
    let date = dateFrom(string: dateString)
    // Normally the install date is set in AppDelegate at launch, doing it manually here.
    Preferences.DAU.installationDate.value = date

    // First ping
    pingWithDateAndCompare(
      dateString: dateString,
      daily: true,
      weekly: true,
      monthly: true,
      first: true,
      dtoi: dateString
    )

    // Subsequent pings should use the same dtoi date
    pingWithDateAndCompare(
      dateString: "2017-11-11",
      daily: true,
      weekly: false,
      monthly: false,
      dtoi: dateString
    )

    pingWithDateAndCompare(
      dateString: "2017-11-20",
      daily: true,
      weekly: true,
      monthly: false,
      dtoi: dateString
    )

    // Exact 30 days after install date
    pingWithDateAndCompare(
      dateString: "2017-12-10",
      daily: true,
      weekly: true,
      monthly: true,
      dtoi: dateString
    )

    XCTAssertNotNil(Preferences.DAU.installationDate.value)

    pingWithDateAndCompare(
      dateString: "2017-12-25",
      daily: true,
      weekly: true,
      monthly: false,
      dtoi: "null"
    )

    // After 30 days installation date pref should be removed.
    XCTAssertNil(Preferences.DAU.installationDate.value)

    pingWithDateAndCompare(
      dateString: "2017-12-28",
      daily: true,
      weekly: false,
      monthly: false,
      dtoi: "null"
    )
  }

  func testDtoiParamExistingUser() {
    // Existing user doesn't have `Preferences.DAU.installationDate` set

    pingWithDateAndCompare(
      dateString: "2017-11-10",
      daily: true,
      weekly: true,
      monthly: true,
      first: true,
      dtoi: "null"
    )

    pingWithDateAndCompare(
      dateString: "2017-12-11",
      daily: true,
      weekly: true,
      monthly: true,
      dtoi: "null"
    )
  }

  func testStatParamsInvalidInputs() {
    XCTAssertNil(dau.dauStatParams(for: date, dauStat: nil, firstPing: false, channel: .beta))
    XCTAssertNil(dau.dauStatParams(for: date, dauStat: nil, firstPing: false, channel: .release))
    XCTAssertNil(dau.dauStatParams(for: date, dauStat: [], firstPing: false, channel: .beta))
  }

  func testFirstLaunch() {
    XCTAssertNil(Preferences.DAU.lastLaunchInfo.value)
    XCTAssertNil(Preferences.DAU.weekOfInstallation.value)
    XCTAssert(Preferences.DAU.firstPingParam.value)

    let dateString = "2017-11-21"
    let date = dateFrom(string: dateString)
    // Normally the install date is set in AppDelegate at launch, doing it manually here.
    Preferences.DAU.installationDate.value = date

    let firstLaunch = pingWithDateAndCompare(
      dateString: dateString,
      daily: true,
      weekly: true,
      monthly: true,
      first: true,
      woi: "2017-11-20",
      dtoi: dateString
    )

    XCTAssertNotNil(firstLaunch)
    XCTAssertNotNil(Preferences.DAU.lastLaunchInfo.value)
    XCTAssertNotNil(Preferences.DAU.weekOfInstallation.value)
    XCTAssertFalse(Preferences.DAU.firstPingParam.value)

    XCTAssertFalse(firstLaunch!.queryParams.contains(URLQueryItem(name: "dtoi", value: "null")))
  }

  func testFirstLaunchUnsuccesfulPing() {
    XCTAssert(Preferences.DAU.firstPingParam.value)

    // First - failed attempt
    pingWithDateAndCompare(
      daily: true,
      weekly: true,
      monthly: true,
      first: true,
      firstPingPref: true
    )

    // First ping is still true
    XCTAssert(Preferences.DAU.firstPingParam.value)

    // Second - succesful attempt
    // Make sure second ping after first failed has `first` param equal true
    pingWithDateAndCompare(daily: true, weekly: true, monthly: true, first: true)

    // Should be false after second successful attempt
    XCTAssertFalse(Preferences.DAU.firstPingParam.value)

    // Third - succesful attempt
    // Finally a non first server ping
    pingWithDateAndCompare(
      dateString: "2020-03-04",
      daily: true,
      weekly: true,
      monthly: true,
      first: false
    )
  }

  func testTwoPingsSameDay() {
    let date = dateFrom(string: "2017-11-20")

    XCTAssertNil(Preferences.DAU.lastLaunchInfo.value)
    XCTAssertNil(Preferences.DAU.weekOfInstallation.value)

    // Acting like a first launch so preferences are going to be set up
    let dauFirstLaunch = DAU()
    let params = dauFirstLaunch.paramsAndPrefsSetup(for: date)

    // These preferences should be set only after a successful ping.
    XCTAssertNil(Preferences.DAU.lastLaunchInfo.value)

    simulatePing(params: params)

    let dauSecondLaunch = DAU()

    XCTAssertNotNil(Preferences.DAU.lastLaunchInfo.value)
    XCTAssertNotNil(Preferences.DAU.weekOfInstallation.value)

    // Second launch on the same day
    let params2 = dauSecondLaunch.paramsAndPrefsSetup(for: date)

    XCTAssertNil(params2)
  }

  func testMondayOfCurrentWeekFormatted() {
    // Making sure gregorian year is showing
    let yearString = Date().mondayOfCurrentWeekFormatted?.truncate(length: 4, trailing: "")
    let year = Int(yearString!)!

    // Verify the year in `mondayOfCurrentWeekFormatted` matches current gregorian calendar year.
    let gregorianCalendar = Calendar(identifier: .gregorian)
    let gregorianCalendarYear = gregorianCalendar.dateComponents([.year], from: Date()).year
    XCTAssertEqual(gregorianCalendarYear, year)
  }

  func testNonDefaultWoiExplicitDate() {
    let correctWoi = "2018-02-26"

    XCTAssertNotEqual(
      correctWoi,
      DAU.defaultWoiDate,
      "woi params must be different from each other for this test"
    )

    pingWithDateAndCompare(
      dateString: "2018-03-04",
      daily: true,
      weekly: true,
      monthly: true,
      first: true,
      woi: correctWoi
    )
    pingWithDateAndCompare(
      dateString: "2018-03-05",
      daily: true,
      weekly: true,
      monthly: false,
      woi: correctWoi
    )
    pingWithDateAndCompare(
      dateString: "2018-03-07",
      daily: true,
      weekly: false,
      monthly: false,
      woi: correctWoi
    )
  }

  func testNonDefaultWoiDefaultConstructor() {
    let dauFirstLaunch = DAU()
    let params = dauFirstLaunch.paramsAndPrefsSetup(for: date)
    XCTAssertFalse(
      params!.queryParams.contains(URLQueryItem(name: "woi", value: DAU.defaultWoiDate))
    )
  }

  func testNotFirstLaunchSetDau() {
    let date = dateFrom(string: "2017-11-20")

    XCTAssertNil(Preferences.DAU.lastLaunchInfo.value)
    XCTAssertNil(Preferences.DAU.weekOfInstallation.value)

    // Acting like a first launch so preferences are going to be set up
    let dauFirstLaunch = DAU()
    let params = dauFirstLaunch.paramsAndPrefsSetup(for: date)

    simulatePing(params: params)

    // Daily check
    pingWithDateAndCompare(
      dateString: "2017-11-22",
      daily: true,
      weekly: false,
      monthly: false,
      woi: woiPrefs
    )
    // Weekly check
    pingWithDateAndCompare(
      dateString: "2017-11-30",
      daily: true,
      weekly: true,
      monthly: false,
      woi: woiPrefs
    )
    // Monthly check
    pingWithDateAndCompare(
      dateString: "2017-12-20",
      daily: true,
      weekly: true,
      monthly: true,
      woi: woiPrefs
    )
  }

  // Tests dau pings at various points of time
  func testLongUseCase() {
    pingWithDateAndCompare(
      dateString: "2018-03-04",
      daily: true,
      weekly: true,
      monthly: true,
      first: true
    )
    pingWithDateAndCompare(dateString: "2018-03-05", daily: true, weekly: true, monthly: false)
    pingWithDateAndCompare(dateString: "2018-03-07", daily: true, weekly: false, monthly: false)
    pingWithDateAndCompare(dateString: "2018-03-11", daily: true, weekly: false, monthly: false)
    pingWithDateAndCompare(dateString: "2018-03-13", daily: true, weekly: true, monthly: false)
    pingWithDateAndCompare(dateString: "2018-03-29", daily: true, weekly: true, monthly: false)
    pingWithDateAndCompare(dateString: "2018-04-29", daily: true, weekly: true, monthly: true)
    pingWithDateAndCompare(dateString: "2019-04-29", daily: true, weekly: true, monthly: true)
    pingWithDateAndCompare(dateString: "2019-04-29", daily: false, weekly: false, monthly: false)
    pingWithDateAndCompare(dateString: "2019-04-29", daily: false, weekly: false, monthly: false)
    pingWithDateAndCompare(dateString: "2019-05-05", daily: true, weekly: false, monthly: true)
    pingWithDateAndCompare(dateString: "2019-05-06", daily: true, weekly: true, monthly: false)
  }

  func testNotFullDayPing() {
    let format = "yyyy-MM-dd, HH:mm"

    pingWithDateAndCompare(
      dateString: "2019-12-31, 16:00",
      daily: true,
      weekly: true,
      monthly: true,
      first: true,
      dateFormat: format
    )
    pingWithDateAndCompare(
      dateString: "2020-01-01, 02:00",
      daily: true,
      weekly: false,
      monthly: true,
      dateFormat: format
    )
    pingWithDateAndCompare(
      dateString: "2020-01-01, 04:00",
      daily: false,
      weekly: false,
      monthly: false,
      dateFormat: format
    )
  }

  func testNotFullDayNoPing() {
    let format = "yyyy-MM-dd, HH:mm"

    pingWithDateAndCompare(
      dateString: "2019-12-31, 16:00",
      daily: true,
      weekly: true,
      monthly: true,
      first: true,
      dateFormat: format
    )
    pingWithDateAndCompare(
      dateString: "2019-12-31, 22:00",
      daily: false,
      weekly: false,
      monthly: false,
      dateFormat: format
    )
    pingWithDateAndCompare(
      dateString: "2020-01-01, 04:00",
      daily: true,
      weekly: false,
      monthly: true,
      dateFormat: format
    )
  }

  func testMondayOfWeek() {
    let monday = dateFrom(string: "2017-11-20")
    XCTAssertEqual(monday.mondayOfCurrentWeekFormatted, "2017-11-20")

    let tuesday = dateFrom(string: "2017-11-21")
    XCTAssertEqual(tuesday.mondayOfCurrentWeekFormatted, "2017-11-20")

    let wednesday = dateFrom(string: "2017-11-22")
    XCTAssertEqual(wednesday.mondayOfCurrentWeekFormatted, "2017-11-20")

    let thursday = dateFrom(string: "2017-11-22")
    XCTAssertEqual(thursday.mondayOfCurrentWeekFormatted, "2017-11-20")

    let friday = dateFrom(string: "2017-12-01")
    XCTAssertEqual(friday.mondayOfCurrentWeekFormatted, "2017-11-27")

    let saturday = dateFrom(string: "2017-12-02")
    XCTAssertEqual(saturday.mondayOfCurrentWeekFormatted, "2017-11-27")

    let sunday = dateFrom(string: "2017-12-03")
    XCTAssertEqual(sunday.mondayOfCurrentWeekFormatted, "2017-11-27")

    let singleDigitTest = dateFrom(string: "2019-02-09")
    XCTAssertEqual(singleDigitTest.mondayOfCurrentWeekFormatted, "2019-02-04")
  }

  func testMigratingInvalidWeekOfInstallPref() throws {
    // (stored, fixed)
    let testValues = [
      ("2023-5-26", "2023-05-26"),
      ("2023-12-6", "2023-12-06"),
      ("2023-5-5", "2023-05-05"),
    ]
    Preferences.DAU.firstPingParam.value = false
    for (storedValue, fixedValue) in testValues {
      Preferences.DAU.weekOfInstallation.value = storedValue
      // Fetching params will trigger migration
      let params = dau.paramsAndPrefsSetup(for: Date())
      XCTAssertEqual(try XCTUnwrap(Preferences.DAU.weekOfInstallation.value), fixedValue)
      Preferences.DAU.weekOfInstallation.reset()
    }
  }

  // No longer valid outside of a hosted app
  // TODO: Refactor DAU to not reach out to Info.plist
  //  func testAPIKeyHeader() throws {
  //    let dau = DAU()
  //    let headers = try XCTUnwrap(dau.paramsAndPrefsSetup(for: date)).headers
  //    XCTAssertEqual(headers["x-brave-api-key"], "key")
  //  }

  // MARK: Helpers

  private func dateFrom(string: String, format: String? = nil) -> Date {
    let dateFormatter = DateFormatter()
    dateFormatter.dateFormat = format ?? "yyyy-MM-dd"
    dateFormatter.timeZone = TimeZone(abbreviation: "GMT")!

    return dateFormatter.date(from: string)!
  }

  private func firstLaunchParam(for isFirst: Bool) -> URLQueryItem {
    return URLQueryItem(name: "first", value: isFirst.description)
  }

  private var woiPrefs: String {
    return Preferences.DAU.weekOfInstallation.value!

  }

  @discardableResult
  private func pingWithDateAndCompare(
    dateString: String = "2017-11-20",
    daily: Bool,
    weekly: Bool,
    monthly: Bool,
    first: Bool = false,
    woi: String? = nil,
    firstPingPref: Bool = false,
    dtoi: String? = nil,
    dateFormat: String? = nil
  ) -> DAU.ParamsAndPrefs? {

    let date = dateFrom(string: dateString, format: dateFormat)
    let dau = DAU()
    let params = dau.paramsAndPrefsSetup(for: date)

    // All dau stats equal false means no ping is send to server
    if daily == false && weekly == false && monthly == false {
      XCTAssertNil(params)
      return params
    }

    XCTAssert(params!.queryParams.contains(URLQueryItem(name: "daily", value: daily.description)))
    XCTAssert(params!.queryParams.contains(URLQueryItem(name: "weekly", value: weekly.description)))
    XCTAssert(
      params!.queryParams.contains(URLQueryItem(name: "monthly", value: monthly.description))
    )
    XCTAssert(params!.queryParams.contains(URLQueryItem(name: "first", value: first.description)))

    if let woi = woi {
      XCTAssert(params!.queryParams.contains(URLQueryItem(name: "woi", value: woi)))
    }

    if let dtoi = dtoi {
      XCTAssert(params!.queryParams.contains(URLQueryItem(name: "dtoi", value: dtoi)))
    }

    simulatePing(firstPing: firstPingPref, params: params!)

    return params
  }

  /// This actually simulates business logic that's done after a successful ping.
  private func simulatePing(firstPing: Bool = false, params: DAU.ParamsAndPrefs?) {
    Preferences.DAU.firstPingParam.value = firstPing

    Preferences.DAU.lastLaunchInfo.value = params!.lastLaunchInfoPreference
  }

  private var appVersion: String {
    let version = AppInfo.appVersion
    return DAU.shouldAppend0(toVersion: AppInfo.appVersion) ? version + ".0" : version
  }

  private func componentsOfDate(_ dateString: String) -> DateComponents {
    let dateFormatter = DateFormatter()
    dateFormatter.dateFormat = "yyyy-MM-dd"

    let date = dateFormatter.date(from: dateString)!

    return (Calendar(identifier: .gregorian) as NSCalendar).components(
      [.day, .month, .year, .weekday],
      from: date
    )
  }
}

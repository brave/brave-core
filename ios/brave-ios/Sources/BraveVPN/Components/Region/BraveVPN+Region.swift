// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import GuardianConnect
import os.log

extension BraveVPN {

  /// Type of the vpn region
  public enum RegionPrecision: Equatable {
    case country, city
  }

  /// List of regions the VPN can connect to
  /// This list is not static and it will be fetcghed with every app launch
  static var allRegions: [GRDRegion] = []

  /// Record last used region
  /// It is used to hold details of the region when automatic selection is used
  static var lastKnownRegion: GRDRegion?

  /// Returns currently chosen region. Returns nil if automatic region is selected.
  public static var selectedRegion: GRDRegion? {
    helper.selectedRegion
  }

  /// Return the region last activated with the details
  /// It will give region details for automatic
  public static var activatedRegion: GRDRegion? {
    helper.selectedRegion ?? lastKnownRegion
  }

  /// Switched to use an automatic region, region closest to user location.
  public static func useAutomaticRegion() {
    helper.select(nil)
  }

  /// Returns true whether automatic region is selected.
  public static var isAutomaticRegion: Bool {
    helper.selectedRegion == nil
  }

  public static func changeVPNRegion(to region: GRDRegion?, completion: @escaping ((Bool) -> Void))
  {
    if isConnected {
      helper.disconnectVPN()
    }

    helper.select(region)

    // The preferred tunnel has to be used for configuration
    // Otherwise faulty configuration will be added while connecting
    let activeTunnelProtocol = GRDTransportProtocol.getUserPreferredTransportProtocol()

    helper.configureFirstTimeUser(for: activeTunnelProtocol, with: region) { status, error in
      let subcredentials =
        "Credentials \(GRDKeychain.getPasswordString(forAccount: kKeychainStr_SubscriberCredential) ?? "Empty")"

      if status == .success {
        Logger.module.debug("Changed VPN region to \(region?.regionName ?? "default selection")")
        completion(true)
      } else {
        Logger.module.debug("Connection failed: \(error?.localizedDescription ?? "nil")")
        Logger.module.debug("Region change connection failed for subcredentials \(subcredentials)")
        completion(false)
      }
    }
  }

  public static func populateRegionDataIfNecessary() {
    helper.setPreferredRegionPrecision(kGRDRegionPrecisionCityByCountry)

    helper.allRegions { regions, error in
      guard let regions = regions else {
        return
      }

      allRegions = regions
    }
  }

  static func changeVPNRegionForPrecision(
    to region: GRDRegion?,
    with precision: RegionPrecision
  ) async -> Bool {

    switch precision {
    case .city:
      helper.setPreferredRegionPrecision(kGRDRegionPrecisionCity)
    default:
      helper.setPreferredRegionPrecision(kGRDRegionPrecisionCountry)
    }

    return await withCheckedContinuation { continuation in
      BraveVPN.changeVPNRegion(to: region) { success in
        continuation.resume(returning: success)
      }
    }
  }

  /// The function that fetched the last used region details from timezones
  /// It used to get details of Region when Automatic Region is used
  /// Otherwise the region detail items will be empty
  /// - Parameter completion: completion block that returns region with details or error
  public static func fetchLastUsedRegionDetail(_ completion: ((GRDRegion?, Bool) -> Void)? = nil) {
    switch vpnState {
    case .expired, .notPurchased:
      break
    case .purchased(_):
      housekeepingApi.requestTimeZonesForRegions { timeZones, error in
        if let error = error {
          logAndStoreError(
            "Failed to get timezones while fetching region: \(error)",
            printToConsole: true
          )
          completion?(nil, false)
        }

        guard let timeZones = timeZones else {
          logAndStoreError(
            "Failed to get timezones while fetching region",
            printToConsole: true
          )
          completion?(nil, false)
          return
        }

        let region = GRDServerManager.localRegion(fromTimezones: timeZones)
        completion?(region, true)
        lastKnownRegion = region
      }
    }
  }
}

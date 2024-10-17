// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import WebKit
import os.log

class BraveSearchResultAdScriptHandler: TabContentScript {
  private struct SearchResultAdResponse: Decodable {
    struct SearchResultAd: Decodable {
      let creativeInstanceId: String
      let placementId: String
      let creativeSetId: String
      let campaignId: String
      let advertiserId: String
      let landingPage: URL
      let headlineText: String
      let description: String
      let rewardsValue: String
      let conversionUrlPatternValue: String?
      let conversionAdvertiserPublicKeyValue: String?
      let conversionObservationWindowValue: Int?
    }

    let creatives: [SearchResultAd]
  }

  static let scriptName = "BraveSearchResultAdScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    return WKUserScript(
      source: secureScript(
        handlerName: messageHandlerName,
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    guard let braveSearchResultAdManager = tab.braveSearchResultAdManager else {
      Logger.module.error("Failed to get brave search result ad handler")
      return
    }

    guard JSONSerialization.isValidJSONObject(message.body),
      let messageData = try? JSONSerialization.data(withJSONObject: message.body, options: []),
      let searchResultAds = try? JSONDecoder().decode(
        SearchResultAdResponse.self,
        from: messageData
      )
    else {
      Logger.module.error("Failed to parse search result ads response")
      return
    }

    processSearchResultAds(searchResultAds, braveSearchResultAdManager: braveSearchResultAdManager)
  }

  private func processSearchResultAds(
    _ searchResultAds: SearchResultAdResponse,
    braveSearchResultAdManager: BraveSearchResultAdManager
  ) {
    for ad in searchResultAds.creatives {
      guard let rewardsValue = Double(ad.rewardsValue)
      else {
        Logger.module.error("Failed to process search result ads JSON-LD")
        return
      }

      var conversion: BraveAds.CreativeSetConversionInfo?
      if let conversionUrlPatternValue = ad.conversionUrlPatternValue,
        let conversionObservationWindowValue = ad.conversionObservationWindowValue
      {
        let timeInterval = TimeInterval(conversionObservationWindowValue) * 1.days
        conversion = .init(
          urlPattern: conversionUrlPatternValue,
          verifiableAdvertiserPublicKeyBase64: ad.conversionAdvertiserPublicKeyValue,
          observationWindow: Date(timeIntervalSince1970: timeInterval)
        )
      }

      let searchResultAd: BraveAds.CreativeSearchResultAdInfo = .init(
        type: .searchResultAd,
        placementId: ad.placementId,
        creativeInstanceId: ad.creativeInstanceId,
        creativeSetId: ad.creativeSetId,
        campaignId: ad.campaignId,
        advertiserId: ad.advertiserId,
        targetUrl: ad.landingPage,
        headlineText: ad.headlineText,
        description: ad.description,
        value: rewardsValue,
        creativeSetConversion: conversion
      )

      braveSearchResultAdManager.triggerSearchResultAdViewedEvent(
        placementId: ad.placementId,
        searchResultAd: searchResultAd
      )
    }
  }
}

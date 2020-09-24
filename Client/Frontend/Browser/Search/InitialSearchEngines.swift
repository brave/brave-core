// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// This is a list of search engines available to the user at first launch.
/// For user controlled search engines class look for `SearchEngines.swift`
class InitialSearchEngines {
    /// Type of search engine available to the user.
    enum SearchEngineID: String {
        case google, bing, duckduckgo, yandex, qwant, startpage, yahoo
    }
    
    struct SearchEngine: Equatable, CustomStringConvertible {
        /// ID of the engine, this is also used to look up the xml file of given search engine if `customId` is not provided.
        let id: SearchEngineID
        /// Some search engines have regional variations which correspond to different xml files in `SearchPlugins` folder.
        /// If you provide this custom id, it will be used instead of `regular` id when accessing the open search xml file.
        var customId: String?
        
        // Only `id` mattera when comparing search engines.
        // This is to prevent adding more than 2 engines of the same type.
        static func == (lhs: Self, rhs: Self) -> Bool {
            lhs.id == rhs.id
        }
        
        var description: String {
            var desc = id.rawValue
            if let customId = customId {
                desc += " with '\(customId)' custom id"
            }
            return desc
        }
    }
    
    private let locale: Locale
    /// List of available engines for given locale. This list is sorted by with priority and default engines at the top.
    var engines: [SearchEngine]
    
    static let ddgDefaultRegions = ["DE", "AU", "NZ", "IE"]
    static let qwantDefaultRegions = ["FR"]
    static let yandexDefaultRegions = ["BY", "KZ", "RU", "TR"]
    static let yahooEligibleRegions =
        ["GB", "US", "AR", "AT", "AU", "BR", "CA", "CH", "CL", "CO", "DE", "DK", "ES", "FI", "FR", "HK",
         "ID", "IE", "IN", "IT", "MX", "MY", "NL", "NO", "NZ", "PE", "PH", "SE", "SG", "TH", "TW", "VE", "VN"]
    
    /// Sets what should be the default search engine for given locale.
    /// If the engine does not exist in `engines` list, it is added to it.
    private(set) var defaultSearchEngine: SearchEngineID {
        didSet {
            if !engines.contains(.init(id: defaultSearchEngine)) {
                // As a fallback we add the missing engine
                engines.append(.init(id: defaultSearchEngine))
            }
        }
    }
    
    /// Sets what should be the default priority engine for given locale.
    /// Priority engines show at the top of search engine onboarding as well as in search engines setting unless user changes search engines order.
    /// If the engine does not exist in `engines` list, it is added to it.
    private(set) var priorityEngine: SearchEngineID? {
        didSet {
            guard let engine = priorityEngine else { return }
            if !engines.contains(.init(id: engine)) {
                // As a fallback we add the missing engine
                engines.append(.init(id: engine))
            }
        }
    }
    
    init(locale: Locale = .current) {
        self.locale = locale
        
        // Default order and available search engines, applies to all locales
        engines = [.init(id: .google),
                   .init(id: .bing),
                   .init(id: .duckduckgo),
                   .init(id: .qwant),
                   .init(id: .startpage)]
        defaultSearchEngine = .google
        
        // Locale and region specific overrides can be modified here.
        // For conflicting rules priorities are as follows:
        // 1. Priority rules, put whatever rules you want there.
        // 2. Region specific rules, this should apply to all rules within a given region,
        //    language code should not matter.
        // 3. Language code rules, region rules can be specified there too(ex. ru_RU)
        languageOverrides()
        regionOverrides()
        priorityOverrides()
        
        // Initial engines should always be sorted so priority and default search engines are at the top,
        // remaining search engines are left in order they were added.
        sortEngines()
    }
    
    // MARK: - Locale overrides
    
    private func languageOverrides() {
        guard let language = locale.languageCode, let region = locale.regionCode else { return }
        if language == "ja" {
            replaceOrInsert(engineId: .yahoo, customId: "yahoo-jp")
        }
        
        if language == "ru" {
            replaceOrInsert(engineId: .yandex, customId: "yandex-ru")
            
            if Self.yandexDefaultRegions.contains(region) {
                defaultSearchEngine = .yandex
            }
        }
        
        if language == "kk" {
            engines.append(.init(id: .yandex))
            
            if Self.yandexDefaultRegions.contains(region) {
                defaultSearchEngine = .yandex
            }
        }
        
        if language == "tr" {
            replaceOrInsert(engineId: .yandex, customId: "yandex-tr")
            
            if Self.yandexDefaultRegions.contains(region) {
                defaultSearchEngine = .yandex
            }
        }
        
    }
    
    private func regionOverrides() {
        guard let region = locale.regionCode else { return }
        
        if Self.ddgDefaultRegions.contains(region) {
            defaultSearchEngine = .duckduckgo
        }
        
        if Self.qwantDefaultRegions.contains(region) {
            defaultSearchEngine = .qwant
        }
    }
    
    private func priorityOverrides() {
        guard let region = locale.regionCode else { return }
        
        if Self.yahooEligibleRegions.contains(region) {
            priorityEngine = .yahoo
        }
    }
    
    // MARK: - Helpers
    
    private func sortEngines() {
        engines = engines
            .sorted { e, _ in e.id == defaultSearchEngine }
            .sorted { e, _ in e.id == priorityEngine }
    }
    
    private func replaceOrInsert(engineId: SearchEngineID, customId: String) {
        guard let engineIndex = engines.firstIndex(of: .init(id: engineId)) else {
            engines.append(.init(id: engineId, customId: customId))
            return
        }
        
        engines[engineIndex] = .init(id: engineId, customId: customId)
    }
}

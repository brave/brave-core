// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Favicon
import Preferences
import Strings
import SwiftUI

/// A snapshot of a `TemplateURL` for display purposes.
///
/// `TemplateURL` objects are only valid until the `TemplateURLService` changes,
/// so the UI holds onto these values instead.
struct SearchEngine: Identifiable, Hashable {
  /// The `TemplateURL.syncGUID`
  var id: String
  var shortName: String
  var url: String
  /// The search results page URL for an empty query, used to fetch the engine's favicon
  var faviconPageURL: URL?
  var isPrepopulated: Bool
}

@MainActor
@Observable
class SearchSettingsViewModel {
  private let templateURLService: any TemplateURLService
  @ObservationIgnored
  private var observer: TemplateURLServiceStateObserver?
  @ObservationIgnored
  private var observation: (any TemplateURLServiceScopedObservation)?

  /// All search engines which can be chosen as the default search engine
  private(set) var searchEngines: [SearchEngine] = []
  /// User added search engines
  private(set) var customSearchEngines: [SearchEngine] = []

  private var storedDefaultSearchEngineID: SearchEngine.ID?
  private var storedDefaultPrivateSearchEngineID: SearchEngine.ID?

  var defaultSearchEngineID: SearchEngine.ID? {
    get { storedDefaultSearchEngineID }
    set {
      guard let newValue, newValue != storedDefaultSearchEngineID else { return }
      templateURLService.setUserSelectedDefaultSearchProvider(withGUID: newValue)
    }
  }

  var defaultPrivateSearchEngineID: SearchEngine.ID? {
    get { storedDefaultPrivateSearchEngineID }
    set {
      guard let newValue, newValue != storedDefaultPrivateSearchEngineID else { return }
      templateURLService.setUserSelectedDefaultPrivateSearchProvider(withGUID: newValue)
      // Changes to the private default are stored in prefs and are not
      // observable via `TemplateURLServiceObserver`
      update()
    }
  }

  init(templateURLService: any TemplateURLService) {
    self.templateURLService = templateURLService
    let observer = TemplateURLServiceStateObserver { [weak self] in
      // `TemplateURLService` notifies observers on the main thread
      MainActor.assumeIsolated {
        self?.update()
      }
    }
    self.observer = observer
    observation = templateURLService.addObserver(observer)
    templateURLService.load()
    update()
  }

  /// Whether or not the engine is the default for either regular or private browsing
  func isDefault(_ engine: SearchEngine) -> Bool {
    engine.id == defaultSearchEngineID || engine.id == defaultPrivateSearchEngineID
  }

  /// Whether a search engine other than `engine` already exists with the same name or search URL
  func containsDuplicate(
    of shortName: String,
    url: String,
    excluding engine: SearchEngine? = nil
  ) -> Bool {
    searchEngines.contains {
      $0.id != engine?.id
        && ($0.shortName.caseInsensitiveCompare(shortName) == .orderedSame || $0.url == url)
    }
  }

  /// Adds a new custom search engine. Returns `false` if it could not be added
  func addSearchEngine(shortName: String, url: String) -> Bool {
    guard let templateURL = URL(string: url) else { return false }
    return templateURLService.addTemplateURL(
      shortName: shortName,
      keyword: TemplateURL.generateKeyword(templateURL),
      url: url,
      suggestionsURL: nil,
      faviconURL: TemplateURL.generateFaviconURL(templateURL)
    ) != nil
  }

  func editSearchEngine(_ engine: SearchEngine, shortName: String, url: String) {
    guard let templateURL = URL(string: url) else { return }
    templateURLService.resetTemplateURL(
      withGUID: engine.id,
      title: shortName,
      keyword: TemplateURL.generateKeyword(templateURL),
      searchURL: url
    )
  }

  /// Deletes a custom search engine, switching the default search engines first if needed.
  func deleteSearchEngine(_ engine: SearchEngine) {
    guard !engine.isPrepopulated else { return }
    let fallbackEngine = searchEngines.first(where: { $0.isPrepopulated && $0.id != engine.id })
    if engine.id == defaultSearchEngineID {
      if let privateID = defaultPrivateSearchEngineID, privateID != engine.id {
        templateURLService.setUserSelectedDefaultSearchProvider(withGUID: privateID)
      } else if let fallbackEngine {
        templateURLService.setUserSelectedDefaultSearchProvider(withGUID: fallbackEngine.id)
      }
    }
    if engine.id == defaultPrivateSearchEngineID,
      let newDefaultID = templateURLService.defaultSearchProvider?.syncGUID ?? fallbackEngine?.id,
      newDefaultID != engine.id
    {
      templateURLService.setUserSelectedDefaultPrivateSearchProvider(withGUID: newDefaultID)
    }
    templateURLService.removeTemplateURL(withGUID: engine.id)
    update()
  }

  private func update() {
    let searchTermsData = templateURLService.searchTermsData
    let engines = templateURLService.templateURLs.map { templateURL in
      SearchEngine(
        id: templateURL.syncGUID,
        shortName: templateURL.shortName,
        url: templateURL.url,
        faviconPageURL: templateURL.urlRef.replaceSearchTerms(
          .init(searchTerms: ""),
          searchTermsData: searchTermsData
        ),
        isPrepopulated: templateURL.isPrepopulated
      )
    }
    // Prepopulated engines are listed first in the order provided by the service
    searchEngines =
      engines.filter(\.isPrepopulated)
      + engines.filter { !$0.isPrepopulated }.sorted {
        $0.shortName.localizedStandardCompare($1.shortName) == .orderedAscending
      }
    customSearchEngines = searchEngines.filter { !$0.isPrepopulated }
    storedDefaultSearchEngineID = templateURLService.defaultSearchProvider?.syncGUID
    storedDefaultPrivateSearchEngineID = templateURLService.defaultPrivateSearchProvider?.syncGUID
  }
}

private class TemplateURLServiceStateObserver: NSObject, TemplateURLServiceObserver {
  private let listener: () -> Void

  init(_ listener: @escaping () -> Void) {
    self.listener = listener
  }

  func templateURLServiceChanged() {
    listener()
  }
}

struct SearchSettingsView: View {
  @Bindable var viewModel: SearchSettingsViewModel
  /// Opens the quick search engines settings
  var openQuickSearchEngines: () -> Void

  @ObservedObject private var showSearchSuggestions = Preferences.Search.showSuggestions
  @ObservedObject private var showRecentSearches = Preferences.Search.shouldShowRecentSearches
  @ObservedObject private var showBrowserSuggestions = Preferences.Search.showBrowserSuggestions

  @State private var customEngineEditorMode: CustomSearchEngineForm.Mode?
  @State private var engineToConfirmDeletion: SearchEngine?

  var body: some View {
    Form {
      Section {
        SearchEnginePicker(
          title: Strings.standardTabSearch,
          searchEngines: viewModel.searchEngines,
          selection: $viewModel.defaultSearchEngineID
        )
        SearchEnginePicker(
          title: Strings.privateTabSearch,
          searchEngines: viewModel.searchEngines,
          selection: $viewModel.defaultPrivateSearchEngineID
        )
        Button {
          openQuickSearchEngines()
        } label: {
          LabeledContent(Strings.quickSearchEngines) {
            Image(systemName: "chevron.right")
              .font(.footnote.weight(.semibold))
              .foregroundStyle(Color(braveSystemName: .iconSecondary))
          }
        }
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        Toggle(Strings.searchSettingSuggestionCellTitle, isOn: $showSearchSuggestions.value)
          .onChange(of: showSearchSuggestions.value) {
            // Setting the value in settings dismisses any opt-in.
            Preferences.Search.shouldShowSuggestionsOptIn.value = false
          }
        Toggle(Strings.searchSettingRecentSearchesCellTitle, isOn: $showRecentSearches.value)
          .onChange(of: showRecentSearches.value) {
            // Setting the value in settings dismisses any opt-in.
            Preferences.Search.shouldShowRecentSearchesOptIn.value = false
          }
        Toggle(isOn: $showBrowserSuggestions.value) {
          Text(Strings.searchSettingBrowserSuggestionCellTitle)
          Text(Strings.searchSettingBrowserSuggestionCellDescription)
        }
      } header: {
        Text(Strings.currentlyUsedSearchEngines)
      }
      .tint(Color(braveSystemName: .primitivePrimary40))

      Section {
        ForEach(viewModel.customSearchEngines) { engine in
          Button {
            customEngineEditorMode = .edit(engine)
          } label: {
            Label {
              Text(engine.shortName)
            } icon: {
              SearchEngineIcon(engine: engine)
            }
          }
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .swipeActions(edge: .trailing) {
            Button(role: .destructive) {
              if viewModel.isDefault(engine) {
                engineToConfirmDeletion = engine
              } else {
                viewModel.deleteSearchEngine(engine)
              }
            } label: {
              Text(Strings.delete)
            }
            Button {
              customEngineEditorMode = .edit(engine)
            } label: {
              Text(Strings.edit)
            }
          }
        }
        Button(Strings.searchSettingAddCustomEngineCellTitle) {
          customEngineEditorMode = .add
        }
      } header: {
        Text(Strings.customSearchEngines)
      }
    }
    .navigationTitle(Strings.searchEngines)
    .sheet(item: $customEngineEditorMode) { mode in
      NavigationStack {
        CustomSearchEngineForm(viewModel: viewModel, mode: mode)
      }
    }
    .alert(
      String(
        format: Strings.CustomSearchEngine.deleteEngineAlertTitle,
        engineToConfirmDeletion?.shortName ?? ""
      ),
      isPresented: Binding(
        get: { engineToConfirmDeletion != nil },
        set: { if !$0 { engineToConfirmDeletion = nil } }
      ),
      presenting: engineToConfirmDeletion
    ) { engine in
      Button(Strings.cancelButtonTitle, role: .cancel) {}
      Button(Strings.delete, role: .destructive) {
        viewModel.deleteSearchEngine(engine)
      }
    } message: { _ in
      Text(Strings.CustomSearchEngine.deleteEngineAlertDescription)
    }
  }
}

/// A row which shows the selected search engine (including its favicon) and
/// lets the user pick another one from a menu.
///
/// A menu-style `Picker` doesn't honor layout modifiers on its options, so the
/// selected engine is displayed by the menu's label instead.
private struct SearchEnginePicker: View {
  var title: String
  var searchEngines: [SearchEngine]
  @Binding var selection: SearchEngine.ID?

  var body: some View {
    Menu {
      Picker(title, selection: $selection) {
        ForEach(searchEngines) { engine in
          Text(engine.shortName)
            .tag(Optional(engine.id))
        }
      }
    } label: {
      HStack(spacing: 8) {
        Text(title)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
        Spacer(minLength: 8)
        if let engine = searchEngines.first(where: { $0.id == selection }) {
          SearchEngineIcon(engine: engine)
          Text(engine.shortName)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
        Image(systemName: "chevron.up.chevron.down")
          .font(.footnote.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .iconSecondary))
      }
      .frame(maxWidth: .infinity, alignment: .leading)
      .fixedSize(horizontal: false, vertical: true)
    }
  }
}

private struct SearchEngineIcon: View {
  var engine: SearchEngine

  var body: some View {
    Group {
      if let url = engine.faviconPageURL {
        FaviconImage(url: url, isPrivateBrowsing: false)
      } else {
        Image(uiImage: Favicon.defaultImage)
          .resizable()
          .aspectRatio(contentMode: .fit)
      }
    }
    .frame(width: 24, height: 24)
    .clipShape(.rect(cornerRadius: 4, style: .continuous))
  }
}

// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import SwiftUI
import Combine
import BraveUI

struct Divided<Content: View, Divider: View>: View {
  var content: Content
  var divider: Divider
  
  init(
    by divider: Divider,
    @ViewBuilder content: () -> Content
  ) {
    self.divider = divider
    self.content = content()
  }
  
  private struct _Layout<Divider: View>: _VariadicView_MultiViewRoot {
    var divider: Divider
    func body(children: _VariadicView.Children) -> some View {
      let last = children.last?.id
      ForEach(children) { child in
        child
        if child.id != last {
          divider
        }
      }
    }
  }
  
  var body: some View {
    _VariadicView.Tree(_Layout(divider: divider)) {
      content
    }
  }
}

struct DestinationLabel<Title: View>: View {
  var image: Image
  var title: Title
  var subtitle: String?
  
  @ScaledMetric private var imageSize: CGFloat = 38.0
  
  init(
    image: Image,
    @ViewBuilder title: () -> Title,
    subtitle: String? = nil
  ) {
    self.image = image
    self.title = title()
    self.subtitle = subtitle
  }
  
  var body: some View {
    HStack(spacing: 12) {
      image
        .foregroundColor(Color(.braveLabel))
        .frame(width: imageSize, height: imageSize)
        .background(Color(.secondaryBraveBackground).clipShape(Circle()))
      VStack(alignment: .leading, spacing: 2) {
        title
          .font(.headline)
          .foregroundColor(Color(.braveLabel))
        if let subtitle {
          Text(subtitle)
            .font(.subheadline)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
      }
    }
    .padding(.vertical, 6)
  }
}

extension DestinationLabel where Title == Text {
  init(
    image: Image,
    title: String,
    subtitle: String? = nil
  ) {
    self.init(image: image, title: { Text(title) }, subtitle: subtitle)
  }
}

private class SearchDelegate: NSObject, UISearchBarDelegate, ObservableObject {
  @Published var query: String = ""
  @Published var isEditing: Bool = false
  
  func searchBarTextDidBeginEditing(_ searchBar: UISearchBar) {
    isEditing = true
  }
  func searchBarTextDidEndEditing(_ searchBar: UISearchBar) {
    isEditing = false
  }
  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    query = searchText
  }
  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    query = ""
  }
}

public class NewsSettingsViewController: UIHostingController<NewsSettingsView> {
  private let dataSource: FeedDataSource
  private let searchDelegate = SearchDelegate()
  private var cancellables: Set<AnyCancellable> = []
  public var viewDidDisappear: (() -> Void)?
  
  public init(dataSource: FeedDataSource) {
    self.dataSource = dataSource
    super.init(rootView: NewsSettingsView(dataSource: dataSource, searchDelegate: searchDelegate))
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    
    if navigationController?.viewControllers.first === self {
      navigationItem.rightBarButtonItem = .init(
        barButtonSystemItem: .done,
        target: self,
        action: #selector(tappedDone)
      )
    }
    navigationItem.largeTitleDisplayMode = .never
    navigationItem.title = "Brave News" // DNT?
    navigationItem.hidesSearchBarWhenScrolling = false
    if Preferences.BraveNews.isEnabled.value {
      navigationItem.searchController = searchController
    }
    if #available(iOS 16.0, *) {
      navigationItem.preferredSearchBarPlacement = .stacked
    }
    
    // Hide the search bar when Brave News is off
    Preferences.BraveNews.isEnabled.$value
      .sink { [weak self] isEnabled in
        guard let self else { return }
        self.navigationItem.searchController = isEnabled ? self.searchController : nil
      }
      .store(in: &cancellables)
    
    // Hide the toolbar when search overlay is visible
    searchDelegate.$query
      .sink { [weak self] query in
        guard let self else { return }
        self.navigationController?.setToolbarHidden(!query.isEmpty, animated: false)
      }
      .store(in: &cancellables)
  }
  
  private lazy var searchController = UISearchController(searchResultsController: nil).then {
    $0.automaticallyShowsCancelButton = true
    $0.hidesNavigationBarDuringPresentation = true
    $0.obscuresBackgroundDuringPresentation = false
    $0.automaticallyShowsSearchResultsController = false
    $0.searchBar.delegate = searchDelegate
    $0.searchBar.placeholder = "Search for news, site, topic or RSS feed" // TODO: Localize
  }
  
  @objc private func tappedDone() {
    dismiss(animated: true)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  public override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    navigationController?.setToolbarHidden(false, animated: animated)
  }
  
  public override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    if navigationController?.isBeingDismissed == false { // Gesture dismiss
      navigationController?.setToolbarHidden(true, animated: animated)
    }
  }
  
  public override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    if navigationController?.isBeingDismissed == true {
      viewDidDisappear?()
    }
  }
}

public struct NewsSettingsView: View {
  @ObservedObject var dataSource: FeedDataSource
  @ObservedObject fileprivate var searchDelegate: SearchDelegate
  @ObservedObject private var isNewsEnabled = Preferences.BraveNews.isEnabled
  @State private var searchResults: SearchResults?
  @State private var isShowingAddSource: Bool = false
//  @State private var findFeedsError: FindFeedsError?
  
  var showBraveNewsToggle: some View {
    Toggle(isOn: $isNewsEnabled.value.animation(.default)) {
      VStack(alignment: .leading, spacing: 4) {
        Text("Show Brave News") // TODO: Localize
          .font(.headline)
          .foregroundColor(Color(.bravePrimary))
        Text("Ad-supported, private and anonymised ads matched on your device.") // TODO: Localize
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))
      }
      .frame(maxWidth: .infinity, alignment: .leading)
    }
    .toggleStyle(SwitchToggleStyle(tint: Color(.braveOrange)))
  }
  
  // Xcode typechecker struggled when these were inside of `body`
  @ViewBuilder var destinations: some View {
    let totalFollowCount = dataSource.followedSources.count + dataSource.followedChannels.count
    NavigationLink {
      SourceListContainerView(dataSource: dataSource)
    } label: {
      DestinationLabel(
        image: Image(braveSystemName: "brave.crown"),
        title: "Popular Sources", // TODO: Localize
        subtitle: "Currently trending sources" // TODO: Localize
      )
    }
    NavigationLink {
      SourceSuggestionsContainerView(dataSource: dataSource)
    } label: {
      DestinationLabel(
        image: Image(braveSystemName: "brave.star"),
        title: "Suggested", // TODO: Localize
        subtitle: "Our curated list of sources"
      )
    }
    if !dataSource.channels.isEmpty {
      NavigationLink {
        ChannelListContainerView(dataSource: dataSource)
      } label: {
        DestinationLabel(
          image: Image(braveSystemName: "brave.newspaper"),
          title: "Channels", // TODO: Localize
          subtitle: "Follow topics relevant to you" // TODO: Localize
        )
      }
    }
    if totalFollowCount > 0 {
      NavigationLink {
        FollowingListContainerView(dataSource: dataSource)
      } label: {
        DestinationLabel(
          image: Image(braveSystemName: "brave.heart"),
          title: {
            HStack {
              Text("Following") // TODO: Localize
              Text(NSNumber(value: totalFollowCount), formatter: NumberFormatter())
                .font(.caption)
                .padding(.vertical, 1)
                .padding(.horizontal, 6)
                .background(Capsule().stroke(Color(.secondaryButtonTint), lineWidth: 1))
                .foregroundColor(Color(.secondaryBraveLabel))
            }
          },
          subtitle: "Manage your following list" // TODO: Localize
        )
      }
    }
  }
  
  public var body: some View {
    List {
      Section {
        if isNewsEnabled.value {
          destinations
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        showBraveNewsToggle
          .padding(.bottom)
          .padding(.vertical)
          .resetListHeaderStyle()
      }
    }
    .animation(.default, value: searchDelegate.isEditing)
    .listBackgroundColor(Color(.braveGroupedBackground))
    .navigationTitle("Brave News") // DNT?
    .navigationBarTitleDisplayMode(.inline)
    .onChange(of: searchDelegate.query) { query in
      searchResults = dataSource.search(query: query)
    }
    .overlay(Group {
      if !searchDelegate.query.isEmpty {
        SearchResultsView(
          results: searchResults ?? .empty,
          isFollowingSource: { dataSource.isFollowingSourceBinding(source: $0) },
          isFollowingChannel: { dataSource.isFollowingChannelBinding(channel: $0) }
        )
      }
    })
    .toolbar {
      ToolbarItemGroup(placement: .bottomBar) {
        Spacer()
        Menu {
          Button {
            isShowingAddSource = true
          } label: {
            Label("Import OPML…", systemImage: "square.and.arrow.down")
          }
        } label: {
          Image(systemName: "ellipsis")
        }
      }
    }
    .opmlImporter(isPresented: $isShowingAddSource, dataSource: dataSource)
  }
}

private struct OPMLImporterViewModifier: ViewModifier {
  @Binding var isPresented: Bool
  var dataSource: FeedDataSource
  @State private var opmlParsedResult: OPMLParsedResult?
  @State private var importError: BraveNewsAddSourceViewController.FindFeedsError? // TODO: Take this out
  
  func body(content: Content) -> some View {
    content
      .fileImporter(
        isPresented: $isPresented,
        allowedContentTypes: [.init("public.opml")!],
        onCompletion: { result in
          switch result {
          case .success(let url):
            importOPML(from: url)
          case .failure:
            break
          }
        }
      )
      .sheet(item: $opmlParsedResult) { result in
        UIKitController(
          UINavigationController(
            rootViewController:
              BraveNewsAddSourceResultsViewController(
                dataSource: dataSource,
                searchedURL: result.url,
                rssFeedLocations: result.locations,
                sourcesAdded: nil
              )
          )
        )
      }
      .alert(item: $importError) { error in
        Alert(
          title: Text(Strings.BraveNews.addSourceFailureTitle),
          message: Text(error.localizedDescription),
          dismissButton: .default(Text(Strings.OKString))
        )
      }
  }
  
  private struct OPMLParsedResult: Hashable, Identifiable {
    var url: URL
    var locations: [RSSFeedLocation]
    var id: String {
      url.absoluteString
    }
  }
  
  private func rssLocationFromOPMLOutline(_ outline: OPML.Outline) -> RSSFeedLocation? {
    guard let url = outline.xmlUrl?.asURL else { return nil }
    return .init(title: outline.text, url: url)
  }
  
  private func importOPML(from url: URL) {
    guard url.isFileURL, let data = try? Data(contentsOf: url) else {
      isPresented = false
      importError = .noFeedsFound
      return
    }
    DispatchQueue.global(qos: .userInitiated).async {
      let opml = OPMLParser.parse(data: data)
      DispatchQueue.main.async {
        guard let opml = opml else {
          isPresented = false
          importError = .invalidData
          return
        }
        let locations = opml.outlines.compactMap(self.rssLocationFromOPMLOutline)
        if locations.isEmpty {
          isPresented = false
          importError = .noFeedsFound
          return
        }
        opmlParsedResult = .init(url: url, locations: locations)
      }
    }
  }
}

extension View {
  func opmlImporter(
    isPresented: Binding<Bool>,
    dataSource: FeedDataSource
  ) -> some View {
    modifier(OPMLImporterViewModifier(isPresented: isPresented, dataSource: dataSource))
  }
}

struct BraveNewsAddSourceView: UIViewControllerRepresentable {
  var dataSource: FeedDataSource
  func makeUIViewController(context: Context) -> UINavigationController {
    UINavigationController(rootViewController: BraveNewsAddSourceViewController(dataSource: dataSource))
  }
  func updateUIViewController(_ uiViewController: UINavigationController, context: Context) {
  }
}

#if DEBUG
struct NewsSettingsView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    UIKitController(
      UINavigationController(
        rootViewController: NewsSettingsViewController(dataSource: .init())
      )
    )
    .accentColor(Color(.braveOrange))
  }
}
#endif

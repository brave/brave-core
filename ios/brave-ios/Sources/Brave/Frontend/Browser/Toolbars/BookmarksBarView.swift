// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import CoreData
import Shared
import UIKit

final class BookmarksBarView: UIView {

  enum UX { static let height: CGFloat = 32 }

  var isEnabled = false {
    didSet {
      updateVisibility()
    }
  }

  private let didSelectBookmark: (Bookmarkv2) -> Void
  private let privateBrowsingManager: PrivateBrowsingManager
  private let stackView = UIStackView()
  private let scrollView = UIScrollView()
  private lazy var heightConstraint = heightAnchor.constraint(equalToConstant: 0)
  private var mobileBookmarksFolder: Bookmarkv2?
  private var bookmarksFRC: BookmarksV2FetchResultsController?
  private var cancellables: Set<AnyCancellable> = []

  init(
    bookmarkManager: BookmarkManager,
    privateBrowsingManager: PrivateBrowsingManager,
    didSelectBookmark: @escaping (Bookmarkv2) -> Void
  ) {
    self.didSelectBookmark = didSelectBookmark
    self.privateBrowsingManager = privateBrowsingManager
    super.init(frame: .zero)

    setupViews()
    updateColors()
    privateBrowsingManager.$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink { [weak self] _ in self?.updateColors() }
      .store(in: &cancellables)

    bookmarkManager.waitForBookmarkModelLoaded { [weak self, weak bookmarkManager] in
      guard let self, let bookmarkManager else { return }
      self.mobileBookmarksFolder = bookmarkManager.mobileNode()
      self.bookmarksFRC = bookmarkManager.frc(parent: self.mobileBookmarksFolder)
      self.bookmarksFRC?.delegate = self
      self.reloadBookmarks()
    }
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupViews() {
    translatesAutoresizingMaskIntoConstraints = false
    clipsToBounds = true
    backgroundColor = .clear

    stackView.axis = .horizontal
    stackView.alignment = .center
    stackView.distribution = .fillProportionally
    stackView.spacing = 2
    stackView.setContentHuggingPriority(.required, for: .horizontal)

    scrollView.showsHorizontalScrollIndicator = false
    scrollView.alwaysBounceHorizontal = true
    scrollView.addSubview(stackView)
    addSubview(scrollView)

    scrollView.translatesAutoresizingMaskIntoConstraints = false
    stackView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      scrollView.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: 12),
      scrollView.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -12),
      scrollView.topAnchor.constraint(equalTo: topAnchor),
      scrollView.bottomAnchor.constraint(equalTo: bottomAnchor),
      stackView.leadingAnchor.constraint(equalTo: scrollView.contentLayoutGuide.leadingAnchor),
      stackView.trailingAnchor.constraint(equalTo: scrollView.contentLayoutGuide.trailingAnchor),
      stackView.topAnchor.constraint(equalTo: scrollView.contentLayoutGuide.topAnchor),
      stackView.bottomAnchor.constraint(equalTo: scrollView.contentLayoutGuide.bottomAnchor),
      stackView.heightAnchor.constraint(equalTo: scrollView.frameLayoutGuide.heightAnchor),
    ])

    stackView.addArrangedSubview(makePlaceholderFolderButton())
    heightConstraint.isActive = true
  }

  private func reloadBookmarks() {
    if let bookmarksFRC {
      do {
        try bookmarksFRC.performFetch()
      } catch {
        assertionFailure("Unable to fetch Mobile Bookmarks: \(error.localizedDescription)")
      }
    }

    stackView.arrangedSubviews.forEach {
      stackView.removeArrangedSubview($0)
      $0.removeFromSuperview()
    }

    if let mobileBookmarksFolder {
      stackView.addArrangedSubview(makeButton(for: mobileBookmarksFolder))
    } else {
      stackView.addArrangedSubview(makePlaceholderFolderButton())
    }

    bookmarksFRC?.fetchedObjects?.forEach { bookmark in
      stackView.addArrangedSubview(makeButton(for: bookmark))
    }

    updateColors()
    updateVisibility()
  }

  private func makeButton(for bookmark: Bookmarkv2) -> UIButton {
    let button = UIButton(type: .system)
    var configuration = UIButton.Configuration.plain()
    configuration.title = bookmark.title ?? bookmark.url
    configuration.image = UIImage(systemName: bookmark.isFolder ? "folder" : "globe")
    configuration.imagePlacement = .leading
    configuration.imagePadding = 5
    configuration.contentInsets = .init(top: 4, leading: 8, bottom: 4, trailing: 8)
    configuration.buttonSize = .small
    button.configuration = configuration
    configureContentWidth(button)
    button.accessibilityLabel = bookmark.title ?? bookmark.url
    button.addAction(UIAction { [weak self] _ in
      self?.didSelectBookmark(bookmark)
    }, for: .touchUpInside)
    return button
  }

  private func makePlaceholderFolderButton() -> UIButton {
    let button = UIButton(type: .system)
    var configuration = UIButton.Configuration.plain()
    configuration.title = Strings.bookmarkRootLevelCellTitle
    configuration.image = UIImage(systemName: "folder")
    configuration.imagePlacement = .leading
    configuration.imagePadding = 5
    configuration.contentInsets = .init(top: 4, leading: 8, bottom: 4, trailing: 8)
    configuration.buttonSize = .small
    button.configuration = configuration
    configureContentWidth(button)
    button.isEnabled = false
    return button
  }

  private func configureContentWidth(_ button: UIButton) {
    button.setContentHuggingPriority(.required, for: .horizontal)
    button.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private func updateVisibility() {
    heightConstraint.constant = isEnabled ? UX.height : 0
    isHidden = !isEnabled
  }

  private func updateColors() {
    let browserColors = privateBrowsingManager.browserColors
    stackView.arrangedSubviews.compactMap { $0 as? UIButton }.forEach { button in
      button.tintColor = browserColors.iconDefault
      button.configuration?.baseForegroundColor = browserColors.textPrimary
    }
  }
}

extension BookmarksBarView: BookmarksV2FetchResultsDelegate {
  func controllerWillChangeContent(_ controller: BookmarksV2FetchResultsController) {}

  func controllerDidChangeContent(_ controller: BookmarksV2FetchResultsController) {}

  func controller(
    _ controller: BookmarksV2FetchResultsController,
    didChange anObject: Any,
    at indexPath: IndexPath?,
    for type: NSFetchedResultsChangeType,
    newIndexPath: IndexPath?
  ) {}

  func controllerDidReloadContents(_ controller: BookmarksV2FetchResultsController) {
    reloadBookmarks()
  }
}

// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Shared
import UIKit
import Web
import os.log

// MARK: - SearchQuickEnginesViewControllerDelegate

protocol SearchQuickEnginesViewControllerDelegate: AnyObject {
  func searchQuickEnginesUpdated()
}

// MARK: - SearchQuickEnginesViewController

class SearchQuickEnginesViewController: UITableViewController {

  // MARK: UX

  struct UX {
    static let iconSize = CGSize(
      width: OpenSearchEngine.preferredIconSize,
      height: OpenSearchEngine.preferredIconSize
    )
  }

  // MARK: Constants

  struct Constants {
    static let quickSearchEngineRowIdentifier = "quickSearchEngineRowIdentifier"
  }

  private var searchEngines: SearchEngines
  private let profile: LegacyBrowserProfile
  weak var delegate: SearchQuickEnginesViewControllerDelegate?

  lazy var addButton = UIBarButtonItem(
    image: UIImage(braveSystemNamed: "leo.plus.add"),
    style: .plain,
    target: self,
    action: #selector(onAddButton)
  )

  lazy var editButton = UIBarButtonItem(
    image: UIImage(braveSystemNamed: "leo.tune"),
    style: .plain,
    target: self,
    action: #selector(onEditButton)
  )

  /// Every quick search engine in its stored order. Disabled engines are included — this screen
  /// is where they are toggled back on.
  private var quickSearchItems: [QuickSearchItem] {
    QuickSearchItem.quickSearchCatalog(
      from: searchEngines.orderedEngines,
      excludingDefault: searchEngines.defaultEngine(forType: .standard)
    )
  }

  // MARK: Lifecycle

  init(profile: LegacyBrowserProfile) {
    self.profile = profile
    self.searchEngines = profile.searchEngines
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    navigationItem.title = Strings.quickSearchEngines

    tableView.do {
      $0.register(
        UITableViewCell.self,
        forCellReuseIdentifier: Constants.quickSearchEngineRowIdentifier
      )
    }

    navigationItem.trailingItemGroups = [
      .init(
        barButtonItems: [editButton, addButton],
        representativeItem: nil
      )
    ]

    editButton.accessibilityLabel = Strings.editQuickSearchEnginesAccessibilityTitle
    addButton.accessibilityLabel =
      Strings.CustomSearchEngine.addCustomSearchEngineAccessibilityTitle
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    tableView.reloadData()
  }

  // MARK: TableViewDataSource - TableViewDelegate

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return quickSearchItems.count
  }

  override func tableView(
    _ tableView: UITableView,
    cellForRowAt indexPath: IndexPath
  ) -> UITableViewCell {
    guard let item = quickSearchItems[safe: indexPath.item] else {
      assertionFailure("No item backing quick search row \(indexPath.item)")
      return UITableViewCell()
    }

    let title: String
    let searchTemplate: String
    let icon: UIImage?
    switch item.source {
    case .engine(let engine):
      title = engine.displayName
      searchTemplate = engine.searchTemplate
      icon = engine.image.createScaled(UX.iconSize)
    case .braveVertical(let vertical):
      title = vertical.displayName
      searchTemplate = vertical.searchTemplate
      icon = UIImage(braveSystemNamed: vertical.iconName)?.createScaled(
        CGSize(width: 26, height: 26)
      )?.withRenderingMode(.alwaysTemplate)
    case .aiChat:
      // Leo is pinned to the row by the view and is not customizable here.
      assertionFailure("Leo in the quick search customize list")
      return UITableViewCell()
    }

    let toggle = UISwitch().then {
      $0.tag = indexPath.item
      $0.addTarget(self, action: #selector(didToggleEngine), for: .valueChanged)
      $0.isOn = item.id.isQuickSearchEnabled
      $0.onTintColor = UIColor(braveSystemName: .primitivePrimary40)
    }

    let searchEngineCell = UITableViewCell(
      style: .subtitle,
      reuseIdentifier: Constants.quickSearchEngineRowIdentifier
    )
    searchEngineCell.do {
      $0.showsReorderControl = true
      $0.accessoryView = toggle
      $0.selectionStyle = .none
      $0.textLabel?.text = title
      $0.textLabel?.adjustsFontSizeToFitWidth = true
      $0.textLabel?.minimumScaleFactor = 0.5
      $0.detailTextLabel?.text = searchTemplateHost(for: searchTemplate)
      $0.detailTextLabel?.textColor = UIColor.secondaryLabel
      $0.imageView?.image = icon
      $0.imageView?.layer.cornerRadius = 4
      $0.imageView?.layer.cornerCurve = .continuous
      $0.imageView?.layer.masksToBounds = true
    }

    if case .braveVertical = item.source, let icon, let imageView = searchEngineCell.imageView {
      imageView.tintColor = .clear
      let bounds = CGRect(origin: .zero, size: icon.size)
      let gradientView = GradientView(braveSystemName: .hero).then {
        $0.frame = bounds
        $0.mask = UIImageView(image: icon).then { $0.frame = bounds }
      }
      imageView.addSubview(gradientView)
    }

    return searchEngineCell
  }

  override func tableView(
    _ tableView: UITableView,
    moveRowAt indexPath: IndexPath,
    to newIndexPath: IndexPath
  ) {
    var items = quickSearchItems
    guard items.indices.contains(indexPath.item), items.indices.contains(newIndexPath.item) else {
      assertionFailure("Reordered a row outside the quick search list")
      // The row already moved on screen, so put it back.
      tableView.reloadData()
      return
    }

    items.insert(items.remove(at: indexPath.item), at: newIndexPath.item)
    items.persistQuickSearchOrder()
    tableView.reloadData()
    delegate?.searchQuickEnginesUpdated()
  }

  override func tableView(
    _ tableView: UITableView,
    editingStyleForRowAt indexPath: IndexPath
  ) -> UITableViewCell.EditingStyle {
    guard case .engine(let engine) = quickSearchItems[safe: indexPath.item]?.source
    else { return .none }
    if engine.isCustomEngine {
      return .delete
    }
    return .none
  }

  override func tableView(
    _ tableView: UITableView,
    commit commitEditingStyle: UITableViewCell.EditingStyle,
    forRowAt forRowAtIndexPath: IndexPath
  ) {
    if commitEditingStyle == .delete,
      let item = quickSearchItems[safe: forRowAtIndexPath.item],
      case .engine(let engine) = item.source
    {
      Task {
        do {
          try await searchEngines.deleteCustomEngine(engine)
          tableView.deleteRows(at: [forRowAtIndexPath], with: .right)
          tableView.reloadData()
          delegate?.searchQuickEnginesUpdated()
        } catch {
          Logger.module.error("Search Engine Error while deleting: \(error)")
        }
      }
    }
  }

  override func tableView(
    _ tableView: UITableView,
    shouldIndentWhileEditingRowAt indexPath: IndexPath
  ) -> Bool {
    return false
  }

  func searchTemplateHost(for template: String) -> String {
    if let queryEndIndex = template.range(of: "?")?.lowerBound {
      return String(template[..<queryEndIndex])
    }
    return template
  }
}

// MARK: - Actions

extension SearchQuickEnginesViewController {

  @objc func didToggleEngine(_ toggle: UISwitch) {
    guard let item = quickSearchItems[safe: toggle.tag] else { return }

    item.id.setQuickSearchEnabled(toggle.isOn)
    delegate?.searchQuickEnginesUpdated()
  }

  @objc func onEditButton() {
    setEditing(!isEditing, animated: true)
    editButton.image = isEditing ? nil : UIImage(braveSystemNamed: "leo.tune")
    editButton.title = isEditing ? Strings.done : nil
    editButton.accessibilityLabel =
      isEditing ? Strings.done : Strings.editQuickSearchEnginesAccessibilityTitle
    addButton.isHidden = isEditing
  }

  @objc func onAddButton() {
    let addCustomSearchEngineVC = CustomEngineViewController(profile: self.profile)
    addCustomSearchEngineVC.onAddSucceed = { [weak self] in
      self?.tableView.reloadData()
      self?.delegate?.searchQuickEnginesUpdated()
    }
    let navVC = UINavigationController(rootViewController: addCustomSearchEngineVC)
    navVC.modalPresentationStyle = .pageSheet
    self.present(navVC, animated: true)
  }
}

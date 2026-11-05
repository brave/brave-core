// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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

  /// Every ordered engine except the standard default, which is not a quick search engine.
  private var quickSearchItems: [QuickSearchItem] {
    var engines = searchEngines.orderedEngines
    if let defaultEngine = searchEngines.defaultEngine(forType: .standard),
      let defaultIndex = engines.firstIndex(where: { $0 === defaultEngine })
    {
      engines.remove(at: defaultIndex)
    }
    return engines.map { QuickSearchItem(source: .engine($0)) }
  }

  /// Maps a row back to its position in `searchEngines.orderedEngines`, which also contains the
  /// default engine and so cannot be indexed by row.
  private func orderedEnginesIndex(forRow row: Int) -> Int? {
    guard case .engine(let engine) = quickSearchItems[safe: row]?.source else { return nil }
    return searchEngines.orderedEngines.firstIndex { $0 === engine }
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
    guard let item = quickSearchItems[safe: indexPath.item],
      case .engine(let engine) = item.source
    else {
      assertionFailure("No engine backing quick search row \(indexPath.item)")
      return UITableViewCell()
    }

    let toggle = UISwitch().then {
      $0.tag = indexPath.item
      $0.addTarget(self, action: #selector(didToggleEngine), for: .valueChanged)
      $0.isOn = searchEngines.isEngineEnabled(engine)
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
      $0.textLabel?.text = engine.displayName
      $0.textLabel?.adjustsFontSizeToFitWidth = true
      $0.textLabel?.minimumScaleFactor = 0.5
      $0.detailTextLabel?.text = searchTemplateHost(for: engine.searchTemplate)
      $0.detailTextLabel?.textColor = UIColor.secondaryLabel
      $0.imageView?.image = engine.image.createScaled(UX.iconSize)
      $0.imageView?.layer.cornerRadius = 4
      $0.imageView?.layer.cornerCurve = .continuous
      $0.imageView?.layer.masksToBounds = true
    }

    return searchEngineCell
  }

  override func tableView(
    _ tableView: UITableView,
    moveRowAt indexPath: IndexPath,
    to newIndexPath: IndexPath
  ) {
    guard let index = orderedEnginesIndex(forRow: indexPath.item),
      let newIndex = orderedEnginesIndex(forRow: newIndexPath.item)
    else {
      assertionFailure("Reordered a row with no backing engine")
      // The row already moved on screen, so put it back.
      tableView.reloadData()
      return
    }

    let engine = searchEngines.orderedEngines.remove(at: index)
    searchEngines.orderedEngines.insert(engine, at: newIndex)
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
      case .engine(let engine) = quickSearchItems[safe: forRowAtIndexPath.item]?.source
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
    guard case .engine(let engine) = quickSearchItems[safe: toggle.tag]?.source
    else { return }

    if toggle.isOn {
      searchEngines.enableEngine(engine)
    } else {
      searchEngines.disableEngine(engine, type: .standard)
    }
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

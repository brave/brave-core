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

    static let headerHeight: CGFloat = 44
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

    let footer = SettingsTableSectionHeaderFooterView(
      frame: CGRect(width: tableView.bounds.width, height: UX.headerHeight)
    )
    tableView.tableFooterView = footer
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    tableView.reloadData()
  }

  // MARK: TableViewDataSource - TableViewDelegate

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return searchEngines.orderedEngines.count - 1
  }

  override func tableView(
    _ tableView: UITableView,
    cellForRowAt indexPath: IndexPath
  ) -> UITableViewCell {
    var engine: OpenSearchEngine?

    // The default engine is not a quick search engine.
    let index = indexPath.item + 1
    engine = searchEngines.orderedEngines[safe: index]

    let toggle = UISwitch().then {
      $0.tag = index
      $0.addTarget(self, action: #selector(didToggleEngine), for: .valueChanged)
      if let searchEngine = engine {
        $0.isOn = searchEngines.isEngineEnabled(searchEngine)
      }
    }

    let searchEngineCell = UITableViewCell(
      style: .subtitle,
      reuseIdentifier: Constants.quickSearchEngineRowIdentifier
    )
    searchEngineCell.do {
      $0.showsReorderControl = true
      $0.accessoryView = toggle
      $0.selectionStyle = .none
      $0.textLabel?.text = engine?.displayName
      $0.textLabel?.adjustsFontSizeToFitWidth = true
      $0.textLabel?.minimumScaleFactor = 0.5
      $0.textLabel?.textColor = UIColor(braveSystemName: .textPrimary)
      $0.detailTextLabel?.text = searchTemplateHost(for: engine?.searchTemplate ?? "")
      $0.detailTextLabel?.textColor = UIColor(braveSystemName: .textSecondary)
      $0.imageView?.image = engine?.image.createScaled(UX.iconSize)
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
    // The first engine (default engine) is not shown in the list, so the indices are off-by-1.
    let index = indexPath.item + 1
    let newIndex = newIndexPath.item + 1
    let engine = searchEngines.orderedEngines.remove(at: index)

    searchEngines.orderedEngines.insert(engine, at: newIndex)
    tableView.reloadData()
  }

  override func tableView(
    _ tableView: UITableView,
    editingStyleForRowAt indexPath: IndexPath
  ) -> UITableViewCell.EditingStyle {
    let index = indexPath.item + 1
    guard let engine = searchEngines.orderedEngines[safe: index]
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
      let engine = searchEngines.orderedEngines[safe: forRowAtIndexPath.item + 1]
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
    let engine = searchEngines.orderedEngines[toggle.tag]

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

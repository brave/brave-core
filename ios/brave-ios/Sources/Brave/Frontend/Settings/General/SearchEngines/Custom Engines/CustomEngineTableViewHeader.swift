// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

class SearchEngineTableViewHeader: UITableViewHeaderFooterView, TableViewReusable {

  // MARK: UX

  struct UX {
    static let addButtonInset: CGFloat = 10
  }

  // MARK: Properties

  var titleLabel = UILabel().then {
    $0.font = UIFont.preferredFont(forTextStyle: .footnote)
    $0.textColor = .secondaryBraveLabel
  }

  lazy var addEngineButton = OpenSearchEngineButton(
    title: Strings.CustomSearchEngine.customEngineAutoAddTitle,
    hidesWhenDisabled: false
  ).then {
    $0.addTarget(self, action: #selector(addEngineAuto), for: .touchUpInside)
    $0.isHidden = true
  }

  var actionHandler: (() -> Void)?

  // MARK: Lifecycle

  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)

    addSubview(titleLabel)
    addSubview(addEngineButton)

    setConstraints()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: Internal

  func setConstraints() {
    titleLabel.snp.makeConstraints { make in
      make.leading.equalTo(readableContentGuide)
      make.bottom.equalToSuperview().inset(4)
    }

    addEngineButton.snp.makeConstraints { make in
      make.trailing.equalTo(readableContentGuide)
      make.centerY.equalToSuperview()
      make.height.equalTo(snp.height)
    }
  }

  // MARK: Actions

  @objc private func addEngineAuto() {
    actionHandler?()
  }
}

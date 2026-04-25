// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Combine
import SnapKit
import SwiftUI
import UIKit

struct WalletListHeaderView<Title: View, Subtitle: View>: View {
  var title: Title
  var subtitle: Subtitle

  init(
    @ViewBuilder title: () -> Title,
    @ViewBuilder subtitle: () -> Subtitle
  ) {
    self.title = title()
    self.subtitle = subtitle()
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 4) {
      title
        .font(.footnote.weight(.medium))
        .textCase(.none)
        .accessibilityAddTraits(.isHeader)
      subtitle
        .font(.caption)
        .textCase(.none)
    }
    .accessibilityElement(children: .contain)
    .foregroundColor(Color(.secondaryBraveLabel))
    .padding(.horizontal, -8)
    .frame(maxWidth: .infinity, alignment: .leading)
  }
}

extension WalletListHeaderView where Title == Text, Subtitle == Text {
  init(title: Text, subtitle: Text) {
    self.title = title
    self.subtitle = subtitle
  }
}

extension WalletListHeaderView where Subtitle == EmptyView {
  init(@ViewBuilder title: () -> Title) {
    self.title = title()
    self.subtitle = EmptyView()
  }
}

extension WalletListHeaderView where Title == Text, Subtitle == EmptyView {
  init(title: Text) {
    self.title = title
    self.subtitle = EmptyView()
  }
}

class WalletTableViewHeaderView: UITableViewHeaderFooterView {
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 6
  }

  let titleLabel = UILabel().then {
    $0.textColor = .secondaryBraveLabel
    $0.font = .preferredFont(for: .footnote, weight: .medium)
    $0.adjustsFontForContentSizeCategory = true
    $0.numberOfLines = 0
  }

  let subtitleLabel = UILabel().then {
    $0.textColor = .secondaryBraveLabel
    $0.font = .preferredFont(forTextStyle: .caption1)
    $0.adjustsFontForContentSizeCategory = true
    $0.numberOfLines = 0
  }

  private var cancellables: Set<AnyCancellable> = []

  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)

    contentView.addSubview(stackView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(subtitleLabel)

    stackView.snp.makeConstraints {
      $0.top.greaterThanOrEqualTo(contentView.layoutMarginsGuide)
      $0.bottom.equalTo(contentView.layoutMarginsGuide)
      $0.leading.trailing.equalTo(contentView).inset(8)
    }

    titleLabel
      .publisher(for: \.text)
      .sink { [weak self] value in
        self?.titleLabel.isHidden = (value ?? "").isEmpty
      }
      .store(in: &cancellables)

    subtitleLabel
      .publisher(for: \.text)
      .sink { [weak self] value in
        self?.subtitleLabel.isHidden = (value ?? "").isEmpty
      }
      .store(in: &cancellables)
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    titleLabel.text = ""
    subtitleLabel.text = ""
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  @available(iOS, unavailable)
  override var textLabel: UILabel? {
    get { nil }
    set {}
  }

  @available(iOS, unavailable)
  override var detailTextLabel: UILabel? {
    get { nil }
    set {}
  }
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class WalletActivityView: SettingsSectionView {
  
  private struct UX {
    static let monthYearColor = Colors.blurple400
  }
  
  let stackView = UIStackView().then {
    $0.axis = .vertical
  }
  
  let monthYearLabel = UILabel().then {
    $0.textColor = UX.monthYearColor
    $0.font = .systemFont(ofSize: 22.0, weight: .medium)
  }
  
  var rows: [RowView] = [] {
    willSet {
      rows.forEach {
        $0.removeFromSuperview()
      }
    }
    didSet {
      rows.forEach {
        stackView.addArrangedSubview($0)
        if $0 !== rows.last {
          stackView.addArrangedSubview(SeparatorView())
        }
      }
      if let disclaimerView = disclaimerView {
        if let finalRow = rows.last {
          stackView.setCustomSpacing(10.0, after: finalRow)
        }
        stackView.addArrangedSubview(disclaimerView)
      }
    }
  }
  
  /// A disclaimer view to show below the rows (Used when the user has auto-contribute enabled
  /// and has a portion of BAT designated to unverified publishers
  var disclaimerView: LinkLabel? {
    willSet {
      disclaimerView?.removeFromSuperview()
    }
    didSet {
      if let disclaimerView = disclaimerView {
        if let finalRow = rows.last {
          stackView.setCustomSpacing(10.0, after: finalRow)
        }
        stackView.addArrangedSubview(disclaimerView)
      }
    }
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(stackView)
    
    stackView.addArrangedSubview(monthYearLabel)
    stackView.setCustomSpacing(8.0, after: monthYearLabel)
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(layoutMarginsGuide)
    }
  }
}

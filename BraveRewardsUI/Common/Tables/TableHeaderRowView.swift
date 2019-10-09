/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class TableHeaderRowView: UIView {
  enum ColumnWidth {
    case percentage(CGFloat)
    case fixed(CGFloat)
  }
  struct Column {
    let title: String
    let width: ColumnWidth
    let align: NSTextAlignment
    init(title: String, width: ColumnWidth, align: NSTextAlignment = .left) {
      self.title = title
      self.width = width
      self.align = align
    }
  }
  
  init(columns: [Column], tintColor: UIColor) {
    super.init(frame: .zero)
    
    backgroundColor = .clear
    
    let stackView = UIStackView()
    let separatorView = SeparatorView().then {
      $0.backgroundColor = tintColor
    }
    addSubview(stackView)
    addSubview(separatorView)
    
    stackView.snp.makeConstraints {
      $0.top.leading.trailing.equalTo(self).inset(15.0)
    }
    separatorView.snp.makeConstraints {
      $0.top.equalTo(stackView.snp.bottom).offset(4.0)
      $0.leading.trailing.equalTo(self)
      $0.bottom.equalTo(self)
    }
    
    for c in columns {
      let label = UILabel().then {
        $0.text = c.title
        $0.appearanceTextColor = tintColor
        $0.font = .systemFont(ofSize: 13.0, weight: .medium)
        $0.textAlignment = c.align
        $0.isAccessibilityElement = !c.title.isEmpty
      }
      stackView.addArrangedSubview(label)
      label.snp.makeConstraints {
        switch c.width {
        case .percentage(let p):
          $0.width.equalTo(stackView).multipliedBy(p)
        case .fixed(let width):
          $0.width.equalTo(width)
        }
      }
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

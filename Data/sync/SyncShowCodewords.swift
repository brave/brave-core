/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveShared

class SyncCodewordList: UIStackView {

    init(words: [String]) {
        super.init(frame: CGRect.zero)

        distribution = .fillEqually
        spacing = 8

        let leftColumn = wordColumnStackView()
        let rightColumn = wordColumnStackView()

        for i in 1...words.count {
            if i <= 8 {
                leftColumn.addArrangedSubview(codewordLabel(words[i-1], order: i))
            } else {
                rightColumn.addArrangedSubview(codewordLabel(words[i-1], order: i))
            }
        }

        addArrangedSubview(leftColumn)
        addArrangedSubview(rightColumn)
    }

    required init(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func wordColumnStackView() -> UIStackView {
        let stackView = UIStackView()
        stackView.axis = .vertical
        stackView.spacing = 4
        stackView.distribution = .fillEqually
        stackView.alignment = .leading

        return stackView
    }

    private func codewordLabel(_ word: String, order: Int) -> UILabel {
        let label = UILabel()
        label.text = "\(order). \(word)"
        label.font = UIFont.systemFont(ofSize: 15, weight: .regular)
        label.textColor = BraveUX.greyJ

        return label
    }
}

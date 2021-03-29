/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Button whose insets are included in its intrinsic size.
 */
class InsetButton: UIButton {
    override var intrinsicContentSize: CGSize {
        let size = super.intrinsicContentSize
        return CGSize(width: size.width + titleEdgeInsets.left + titleEdgeInsets.right,
            height: size.height + titleEdgeInsets.top + titleEdgeInsets.bottom)
    }
    
    func addTrailingImageIcon(image: UIImage, inset: CGFloat = 15) {
        let imageView = UIImageView(image: image).then {
            $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
        }

        addSubview(imageView)
        titleEdgeInsets.right += inset
        
        imageView.snp.makeConstraints {
            if let titleView = titleLabel {
                $0.leading.equalTo(titleView.snp.trailing).inset(-inset)
                $0.centerY.equalTo(titleView.snp.centerY)
            } else {
                $0.leading.equalToSuperview().inset(inset)
                $0.centerY.equalToSuperview()
            }
            
            $0.trailing.equalToSuperview().inset(inset)
            $0.width.equalTo(inset)
            $0.height.equalTo(inset)
        }
    }
}

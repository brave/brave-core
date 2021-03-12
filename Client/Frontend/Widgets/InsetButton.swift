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
    
    func addTrailingImageIcon(image: UIImage, offset: CGFloat = 15) {
        let imageView = UIImageView(image: image)
        imageView.translatesAutoresizingMaskIntoConstraints = false

        addSubview(imageView)
        titleEdgeInsets.right += offset

        NSLayoutConstraint.activate([
            imageView.leadingAnchor.constraint(equalTo: titleLabel?.trailingAnchor ?? trailingAnchor, constant: offset),
            imageView.centerYAnchor.constraint(equalTo: titleLabel?.centerYAnchor ?? centerYAnchor),
            imageView.widthAnchor.constraint(equalToConstant: offset),
            imageView.heightAnchor.constraint(equalToConstant: offset)
        ])
    }
}

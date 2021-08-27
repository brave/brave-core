// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SDWebImage
import AVFoundation

class PlaylistAssetFetcher {
    private let asset: AVURLAsset
    
    init(asset: AVURLAsset) {
        self.asset = asset
    }
    
    func cancelLoading() {
        asset.cancelLoading()
    }
}

class PlaylistResizingThumbnailView: UIImageView {
    private var onImageChanged: (PlaylistResizingThumbnailView) -> Void
    
    init(onImageChanged: @escaping (PlaylistResizingThumbnailView) -> Void) {
        self.onImageChanged = onImageChanged
        super.init(frame: .zero)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override var image: UIImage? {
        didSet {
            onImageChanged(self)
        }
    }
}

class PlaylistCell: UITableViewCell {
    let thumbnailGenerator = PlaylistThumbnailRenderer()
    var durationFetcher: PlaylistAssetFetcher?
    
    private let thumbnailMaskView = CAShapeLayer().then {
        $0.fillColor = UIColor.white.cgColor
    }
    
    private let thumbnailHolder = UIView().then {
        $0.backgroundColor = .black
        $0.contentMode = .scaleAspectFit
        $0.layer.cornerRadius = 5.0
        $0.layer.cornerCurve = .continuous
        $0.layer.masksToBounds = true
    }
    
    let thumbnailActivityIndicator = UIActivityIndicatorView(style: .medium).then {
        $0.hidesWhenStopped = true
        $0.tintColor = .white
    }
    
    let thumbnailView = PlaylistResizingThumbnailView(onImageChanged: {
        onThumbnailChanged($0)
    }).then {
        $0.contentMode = .scaleAspectFit
        $0.layer.cornerRadius = 5.0
        $0.layer.cornerCurve = .continuous
        $0.layer.masksToBounds = true
    }
    
    let titleLabel = UILabel().then {
        $0.textColor = .white
        $0.numberOfLines = 2
        $0.font = .systemFont(ofSize: 16.0, weight: .medium)
    }
    
    let detailLabel = UILabel().then {
        $0.textColor = #colorLiteral(red: 0.5254901961, green: 0.5568627451, blue: 0.5882352941, alpha: 1)
        $0.font = .systemFont(ofSize: 14.0, weight: .regular)
    }
    
    private let iconStackView = UIStackView().then {
        $0.alignment = .center
        $0.spacing = 15.0
    }
    
    private let infoStackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .top
        $0.spacing = 5.0
    }
    
    private let separator = UIView().then {
        $0.backgroundColor = UIColor(white: 1.0, alpha: 0.15)
    }
    
    func prepareForDisplay() {
        thumbnailGenerator.cancel()
        thumbnailView.cancelFaviconLoad()
        durationFetcher?.cancelLoading()
        durationFetcher = nil
    }
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        
        preservesSuperviewLayoutMargins = false
        selectionStyle = .none
        
        contentView.addSubview(iconStackView)
        contentView.addSubview(infoStackView)
        iconStackView.addArrangedSubview(thumbnailHolder)
        infoStackView.addArrangedSubview(titleLabel)
        infoStackView.addArrangedSubview(detailLabel)
        contentView.addSubview(separator)
        thumbnailHolder.addSubview(thumbnailView)
        thumbnailHolder.addSubview(thumbnailActivityIndicator)
        
        thumbnailHolder.snp.makeConstraints {
            // Keeps a 94.0px width on iPhone-X as per design
            $0.width.equalTo(iconStackView.snp.height).multipliedBy(1.46875 /* 94.0 / (tableViewCellHeight - (8.0 * 2)) */)
            $0.height.equalToSuperview()
        }
        
        thumbnailView.snp.makeConstraints {
            $0.center.equalToSuperview()
            $0.leading.trailing.top.bottom.equalToSuperview().priority(.high)
            $0.width.height.equalToSuperview()
        }
        
        thumbnailActivityIndicator.snp.makeConstraints {
            $0.center.equalToSuperview()
        }
        
        iconStackView.snp.makeConstraints {
            $0.leading.equalToSuperview().offset(12.0)
            $0.top.bottom.equalToSuperview().inset(8.0)
        }
        
        infoStackView.snp.makeConstraints {
            $0.leading.equalTo(iconStackView.snp.trailing).offset(8.0)
            $0.trailing.equalToSuperview().offset(-15.0)
            $0.centerY.equalToSuperview()
            $0.top.greaterThanOrEqualTo(iconStackView.snp.top)
            $0.bottom.lessThanOrEqualTo(iconStackView.snp.bottom)
        }
        
        separator.snp.makeConstraints {
            $0.leading.equalTo(titleLabel.snp.leading)
            $0.trailing.bottom.equalToSuperview()
            $0.height.equalTo(1.0 / UIScreen.main.scale)
        }
    }
    
    private static func onThumbnailChanged(_ imageView: PlaylistResizingThumbnailView) {
        guard let superView = imageView.superview else { return }
        
        imageView.snp.remakeConstraints {
            $0.center.equalToSuperview()
            
            if let size = imageView.image?.size {
                if size.width > superView.bounds.width || size.height > superView.bounds.height {
                    $0.width.height.equalToSuperview()
                } else {
                    $0.leading.trailing.top.bottom.equalToSuperview().priority(.high)
                    $0.width.height.equalTo(28.0)
                }
            } else {
                $0.width.height.equalToSuperview()
            }
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override var layoutMargins: UIEdgeInsets {
        get {
            return .zero
        }

        set { // swiftlint:disable:this unused_setter_value
            super.layoutMargins = .zero
        }
    }
    
    override var separatorInset: UIEdgeInsets {
        get {
            return UIEdgeInsets(top: 0, left: self.titleLabel.frame.origin.x, bottom: 0, right: 0)
        }
        
        set { // swiftlint:disable:this unused_setter_value
            super.separatorInset = UIEdgeInsets(top: 0, left: self.titleLabel.frame.origin.x, bottom: 0, right: 0)
        }
    }
}

// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import AVKit

class VideoPlayerControlsView: UIView {
    private let blurView = UIVisualEffectView(effect: UIBlurEffect(style: .systemMaterialDark)).then {
        $0.contentView.backgroundColor = #colorLiteral(red: 0.231372549, green: 0.2431372549, blue: 0.3098039216, alpha: 0.8)
    }
    
    private let topControlsStackView = UIStackView().then {
        $0.distribution = .fillEqually
    }
    
    let repeatButton = UIButton().then {
        $0.imageView?.contentMode = .scaleAspectFit
        $0.setImage(#imageLiteral(resourceName: "playlist_repeat"), for: .normal)
    }
    
    let skipBackButton = UIButton().then {
        $0.imageView?.contentMode = .scaleAspectFit
        $0.setImage(#imageLiteral(resourceName: "playlist_rewind"), for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 10.0)
    }
    
    let skipForwardButton = UIButton().then {
        $0.imageView?.contentMode = .scaleAspectFit
        $0.setImage(#imageLiteral(resourceName: "playlist_forward"), for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 10.0)
    }
    
    let playPauseButton = UIButton().then {
        $0.imageView?.contentMode = .scaleAspectFit
        $0.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)
    }
    
    let nextButton = UIButton().then {
        $0.imageView?.contentMode = .scaleAspectFit
        $0.setImage(#imageLiteral(resourceName: "playlist_next"), for: .normal)
    }
    
    let playbackRateButton = UIButton().then {
        $0.imageView?.contentMode = .scaleAspectFit
        $0.setTitle("1x", for: .normal)
        $0.setTitleColor(.white, for: .normal)
        $0.titleLabel?.font = .systemFont(ofSize: 21.0, weight: .medium)
    }
    
    let castButton = UIButton().then {
        $0.imageView?.contentMode = .scaleAspectFit
        $0.setImage(#imageLiteral(resourceName: "playlist_airplay"), for: .normal)
        
        let routePicker = AVRoutePickerView()
        routePicker.tintColor = .clear
        routePicker.activeTintColor = .clear
        routePicker.prioritizesVideoDevices = true
        
        $0.addSubview(routePicker)
        routePicker.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    
    let trackBar = VideoTrackerBar()
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        addSubview(blurView)
        addSubview(topControlsStackView)
        [repeatButton, skipBackButton, playPauseButton, skipForwardButton, nextButton].forEach({
            topControlsStackView.addArrangedSubview($0)
        })
        addSubview(playbackRateButton)
        addSubview(trackBar)
        addSubview(castButton)
        
        blurView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        topControlsStackView.snp.makeConstraints {
            $0.leading.equalToSuperview().priority(.high)
            $0.trailing.equalToSuperview().priority(.high)
            $0.width.equalTo(350.0).priority(.required)
            $0.top.equalToSuperview().inset(20.0)
            $0.bottom.equalTo(self.snp.centerY).offset(-4.0)
            $0.centerX.equalToSuperview()
        }
        
        playbackRateButton.snp.makeConstraints {
            $0.leading.equalToSuperview().inset(15.0)
            $0.centerY.equalTo(trackBar.snp.centerY)
        }
        
        castButton.snp.makeConstraints {
            $0.trailing.equalToSuperview().inset(15.0)
            $0.centerY.equalTo(trackBar.snp.centerY)
        }
        
        trackBar.snp.makeConstraints {
            $0.leading.equalTo(playbackRateButton.snp.trailing).offset(15.0)
            $0.trailing.equalTo(castButton.snp.leading).offset(-15.0)
            $0.top.equalTo(self.snp.centerY).offset(4.0)
            $0.bottom.equalToSuperview().offset(-12.0)
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}

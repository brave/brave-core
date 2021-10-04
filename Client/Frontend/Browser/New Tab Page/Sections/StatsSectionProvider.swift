// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared
import BraveUI

class StatsSectionProvider: NSObject, NTPSectionProvider {
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return 1
    }
    
    func registerCells(to collectionView: UICollectionView) {
        collectionView.register(NewTabCollectionViewCell<BraveShieldStatsView>.self)
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        return collectionView.dequeueReusableCell(for: indexPath) as NewTabCollectionViewCell<BraveShieldStatsView>
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
        var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
        size.height = 110
        return size
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
        return UIEdgeInsets(top: 0, left: 20, bottom: 0, right: 20)
    }
}

class BraveShieldStatsView: UIView {
    fileprivate let millisecondsPerItem = 50
    fileprivate let bytesPerItem = 30485
    
    private lazy var adsStatView: StatView = {
        let statView = StatView(frame: CGRect.zero)
        statView.title = Strings.shieldsAdAndTrackerStats.capitalized
        statView.color = .statsAdsBlockedTint
        return statView
    }()
    
    private lazy var dataSavedStatView: StatView = {
        let statView = StatView(frame: .zero)
        statView.title = Strings.dataSavedStat
        statView.color = .statsDataSavedTint
        return statView
    }()
    
    private lazy var timeStatView: StatView = {
        let statView = StatView(frame: .zero)
        statView.title = Strings.shieldsTimeStats
        statView.color = .statsTimeSavedTint
        return statView
    }()
    
    private lazy var stats: [StatView] = {
        return [self.adsStatView, self.dataSavedStatView, self.timeStatView]
    }()
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        for s: StatView in stats {
            addSubview(s)
        }
        
        update()
        
        NotificationCenter.default.addObserver(self, selector: #selector(update), name: NSNotification.Name(rawValue: BraveGlobalShieldStats.didUpdateNotification), object: nil)
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    deinit {
        NotificationCenter.default.removeObserver(self)
    }
    
    override func layoutSubviews() {
        let width: CGFloat = frame.width / CGFloat(stats.count)
        var offset: CGFloat = 0
        for s: StatView in stats {
            var f: CGRect = s.frame
            f.origin.x = offset
            f.size = CGSize(width: width, height: frame.height)
            s.frame = f
            offset += width
        }
    }
    
    @objc private func update() {
        DispatchQueue.main.async { [weak self] in
            guard let self = self else { return }
            self.adsStatView.stat = (BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection).kFormattedNumber
            self.dataSavedStatView.stat = self.dataSaved
            self.timeStatView.stat = self.timeSaved
        }
    }
    
    var dataSaved: String {
        var estimatedDataSavedInBytes = (BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection) * bytesPerItem
        
        if estimatedDataSavedInBytes <= 0 { return "0" }
        let _1MB = 1000 * 1000
        
        // Byte formatted megabytes value can be too long to display nicely(#3274).
        // As a workaround we cut fraction value from megabytes by rounding it down.
        if estimatedDataSavedInBytes > _1MB {
            estimatedDataSavedInBytes = (estimatedDataSavedInBytes / _1MB) * _1MB
        }
        
        let formatter = ByteCountFormatter().then {
            $0.allowsNonnumericFormatting = false
            // Skip bytes, because it displays as `bytes` instead of `B` which is too long for our shield stat.
            // This is for extra safety only, there's not many sub 1000 bytes tracker scripts in the wild.
            $0.allowedUnits = [.useKB, .useMB, .useGB, .useTB]
        }
        
        return formatter.string(fromByteCount: Int64(estimatedDataSavedInBytes))
    }
    
    var timeSaved: String {
        let estimatedMillisecondsSaved = (BraveGlobalShieldStats.shared.adblock + BraveGlobalShieldStats.shared.trackingProtection) * millisecondsPerItem
        let hours = estimatedMillisecondsSaved < 1000 * 60 * 60 * 24
        let minutes = estimatedMillisecondsSaved < 1000 * 60 * 60
        let seconds = estimatedMillisecondsSaved < 1000 * 60
        var counter: Double = 0
        var text = ""
        
        if seconds {
            counter = ceil(Double(estimatedMillisecondsSaved / 1000))
            text = Strings.shieldsTimeStatsSeconds
        } else if minutes {
            counter = ceil(Double(estimatedMillisecondsSaved / 1000 / 60))
            text = Strings.shieldsTimeStatsMinutes
        } else if hours {
            counter = ceil(Double(estimatedMillisecondsSaved / 1000 / 60 / 60))
            text = Strings.shieldsTimeStatsHour
        } else {
            counter = ceil(Double(estimatedMillisecondsSaved / 1000 / 60 / 60 / 24))
            text = Strings.shieldsTimeStatsDays
        }
        
        if let counterLocaleStr = Int(counter).decimalFormattedString {
            return counterLocaleStr + text
        } else {
            return "0" + Strings.shieldsTimeStatsSeconds     // If decimalFormattedString returns nil, default to "0s"
        }
    }
}

private class StatView: UIView {
    var color: UIColor = .braveLabel {
        didSet {
            statLabel.textColor = color
        }
    }
    
    var stat: String = "" {
        didSet {
            statLabel.text = "\(stat)"
        }
    }
    
    var title: String = "" {
        didSet {
            titleLabel.text = "\(title)"
        }
    }
    
    fileprivate var statLabel: UILabel = {
        let label = UILabel()
        label.textAlignment = .center
        label.font = .systemFont(ofSize: 32, weight: UIFont.Weight.medium)
        label.minimumScaleFactor = 0.5
        label.adjustsFontSizeToFitWidth = true
        return label
    }()
    
    fileprivate var titleLabel: UILabel = {
        let label = UILabel()
        label.textColor = .white
        label.textAlignment = .center
        label.numberOfLines = 0
        label.font = UIFont.systemFont(ofSize: 10, weight: UIFont.Weight.medium)
        return label
    }()
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        addSubview(statLabel)
        addSubview(titleLabel)
        
        statLabel.snp.makeConstraints({ (make) -> Void in
            make.left.equalTo(0)
            make.right.equalTo(0)
            make.centerY.equalTo(self).offset(-(statLabel.sizeThatFits(CGSize(width: CGFloat.greatestFiniteMagnitude, height: CGFloat.greatestFiniteMagnitude)).height)-10)
        })
        
        titleLabel.snp.makeConstraints({ (make) -> Void in
            make.left.equalTo(0)
            make.right.equalTo(0)
            make.top.equalTo(statLabel.snp.bottom).offset(5)
        })
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}

private extension Int {
    var decimalFormattedString: String? {
        let numberFormatter = NumberFormatter()
        numberFormatter.numberStyle = NumberFormatter.Style.decimal
        numberFormatter.locale = NSLocale.current
        return numberFormatter.string(from: self as NSNumber)
    }
}

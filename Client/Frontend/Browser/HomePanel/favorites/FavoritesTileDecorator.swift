/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Storage
import Shared
import Data

private let log = Logger.browserLogger

struct FallbackIconUX {
    static let minSize = CGSize(width: 120, height: 120)
    static let size = CGSize(width: 250, height: 250)
    static let color: UIColor = UX.GreyE
}

enum FavoritesTileType {
    /// Predefinied tile color and custom icon, used for most popular websites.
    case preset
    /// Just a favicon, no background color.
    case faviconOnly
    /// A globe icon, no background color.
    case defaultTile
}

class FavoritesTileDecorator {
    let url: URL
    let normalizedHost: String
    let cell: ThumbnailCell
    let indexPath: IndexPath
    let color: UIColor?
    weak var collection: UICollectionView?

    /// Returns SuggestedSite for given tile or nil if no suggested sites found.
    var commonWebsite: SuggestedSite? {
        let suggestedSites = SuggestedSites.asArray()

        return suggestedSites.filter { site in
            extractDomainURL(site.url) == normalizedHost
            }.first
    }

    var tileType: FavoritesTileType {
        if commonWebsite != nil {
            return .preset
        } else if ImageCache.shared.hasImage(url, type: .square) {
            return .faviconOnly
        } else {
            return .defaultTile
        }
    }

    init(url: URL, cell: ThumbnailCell, indexPath: IndexPath, color: UIColor? = nil) {
        self.url = url
        self.cell = cell
        self.indexPath = indexPath
        self.color = color
        normalizedHost = url.normalizedHost ?? ""
    }

    func decorateTile() {
        switch tileType {
        case .preset:
            guard let website = commonWebsite, let iconUrl = website.wordmark.url.asURL, let host = iconUrl.host,
                iconUrl.scheme == "asset", let image = UIImage(named: host) else {
                    // FIXME: Split it into separate guard clauses to give more specific error logs in case something is missing?
                    log.warning("website, iconUrl, host, or image is nil, using default tile")
                    setDefaultTile()
                    return
            }

            cell.imageView.backgroundColor = website.backgroundColor
            cell.imageView.contentMode = .scaleAspectFit
            cell.imageView.layer.minificationFilter = kCAFilterTrilinear
            cell.showBorder(!UIApplication.isInPrivateMode)

            UIGraphicsBeginImageContextWithOptions(image.size, false, 0)
            image.draw(in: CGRect(origin: CGPoint(x: 3, y: 6), size: CGSize(width: image.size.width - 6, height: image.size.height - 6)))
            let scaledImage = UIGraphicsGetImageFromCurrentImageContext()
            UIGraphicsEndImageContext()
            cell.imageView.image = scaledImage
            break
        case .faviconOnly:
            ImageCache.shared.image(url, type: .square, callback: { (image) in
                DispatchQueue.main.async {
                    self.cell.imageView.image = image
                }
            })
            break
        case .defaultTile:
            setDefaultTile()

            // attempt to resolove domain problem
            let context = DataController.shared.mainThreadContext
            if let domain = Domain.getOrCreateForUrl(url, context: context), let faviconMO = domain.favicon, let urlString = faviconMO.url, let iconUrl = URL(string: urlString) {
                DispatchQueue.main.async {
                    self.setCellImage(self.cell, iconUrl: iconUrl, cacheWithUrl: self.url)
                }
            }
            else {
                // last resort - download the icon
                downloadFaviconsAndUpdateForUrl(url, indexPath: indexPath)
            }
            break
        }
    }

    private func setDefaultTile() {
        cell.imageView.image = ThumbnailCellUX.PlaceholderImage
    }

    fileprivate func setCellImage(_ cell: ThumbnailCell, iconUrl: URL, cacheWithUrl: URL) {
        ImageCache.shared.image(cacheWithUrl, type: .square, callback: { (image) in
            if image != nil {
                DispatchQueue.main.async {
                    cell.imageView.image = image
                }
            }
            else {
                DispatchQueue.main.async {
                    cell.imageView.sd_setImage(with: iconUrl, completed: { (img, err, type, url) in
                        var finalImage = img
                        var useFallback = false

                        if img == nil {
                            useFallback = true
                        } else if let img = img, img.size.width < FallbackIconUX.minSize.width && img.size.height < FallbackIconUX.minSize.height {
                            useFallback = true
                        }

                        if useFallback, let host = self.url.host, let letter = host.replacingOccurrences(of: "www.", with: "").first {
                            var tabColor = FallbackIconUX.color
                            
                            // Only use stored color if it's not too light.
                            if let color = self.color, !color.isLight {
                                tabColor = color
                            }
                            
                            finalImage = FavoritesHelper.fallbackIcon(withLetter: String(letter), color: tabColor, andSize: FallbackIconUX.size)
                        }

                        if let finalImage = finalImage {
                            ImageCache.shared.cache(finalImage, url: cacheWithUrl, type: .square, callback: nil)
                            cell.imageView.image = finalImage
                        }
                    })
                }
            }
        })
    }

    fileprivate func downloadFaviconsAndUpdateForUrl(_ url: URL, indexPath: IndexPath) {
        weak var weakSelf = self
        
        guard let appDelegate = UIApplication.shared.delegate as? AppDelegate, let profile = appDelegate.profile else {
            return
        }
        
        FaviconFetcher.getForURL(url, profile: profile).uponQueue(DispatchQueue.main) { result in
            guard let favicons = result.successValue, favicons.count > 0, let foundIconUrl = favicons.first?.url.asURL,
                let cell = weakSelf?.collection?.cellForItem(at: indexPath) as? ThumbnailCell else { return }
            self.setCellImage(cell, iconUrl: foundIconUrl, cacheWithUrl: url)
        }
    }

    private func extractDomainURL(_ url: String) -> String {
        return URL(string: url)?.normalizedHost ?? url
    }
}

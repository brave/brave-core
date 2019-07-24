/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Storage
import Shared
import Data

extension UIImageView {

    public func setIcon(_ icon: Favicon?, forURL url: URL?, scaledDefaultIconSize: CGSize? = nil, completed completionBlock: ((UIColor, URL?) -> Void)? = nil) {
        if let url = url, icon == nil {
            let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
            let domain = Domain.getOrCreate(forUrl: url, persistent: !isPrivateBrowsing)
            if let favicon = domain.favicon {
                setIconURL(favicon.url, forURL: url, completed: completionBlock, scaledDefaultIconSize: scaledDefaultIconSize)
                return
            }
        }
        setIconURL(icon?.url, forURL: url, completed: completionBlock, scaledDefaultIconSize: scaledDefaultIconSize)
    }
    
    public func setIconMO(_ icon: FaviconMO?, forURL url: URL?, scaledDefaultIconSize: CGSize? = nil, completed completionBlock: ((UIColor, URL?) -> Void)? = nil) {
        setIconURL(icon?.url, forURL: url, completed: completionBlock, scaledDefaultIconSize: scaledDefaultIconSize)
    }
    
    private func setIconURL(_ iconURL: String?, forURL url: URL?, completed completionBlock: ((UIColor, URL?) -> Void)?, scaledDefaultIconSize: CGSize? = nil) {
        if let url = url, let defaultIcon = FaviconFetcher.getDefaultIconForURL(url: url), iconURL == nil {
            if let scaleToSize = scaledDefaultIconSize {
                self.image = UIImage(contentsOfFile: defaultIcon.url)?.createScaled(scaleToSize)
                self.contentMode = .center
            } else {
                self.image = UIImage(contentsOfFile: defaultIcon.url)
            }
            self.backgroundColor = defaultIcon.color
            completionBlock?(defaultIcon.color, url)
        } else {
            let defaults = defaultFavicon(url)
            if let url = url, iconURL == nil {
                FaviconFetcher.getForURL(url).uponQueue(.main) { result in
                    guard let favicons = result.successValue, favicons.count > 0, let foundIconUrl = favicons.first?.url.asURL else {
                        return
                    }
                    self.sd_setImage(with: foundIconUrl, placeholderImage: defaults.image, options: []) {(img, err, _, _) in
                        guard let image = img, err == nil else {
                            self.backgroundColor = defaults.color
                            completionBlock?(defaults.color, url)
                            return
                        }
                        self.color(forImage: image, andURL: url, completed: completionBlock)
                    }
                }
                return
            }
            let imageURL = URL(string: iconURL ?? "")
            self.sd_setImage(with: imageURL, placeholderImage: defaults.image, options: []) {(img, err, _, _) in
                guard let image = img, let dUrl = url, err == nil else {
                    self.backgroundColor = defaults.color
                    completionBlock?(defaults.color, url)
                    return
                }
                self.color(forImage: image, andURL: dUrl, completed: completionBlock)
            }
        }
    }

   /*
    * Fetch a background color for a specfic favicon UIImage. It uses the URL to store the UIColor in memory for subsequent requests.
    */
    private func color(forImage image: UIImage, andURL url: URL, completed completionBlock: ((UIColor, URL?) -> Void)? = nil) {
        guard let domain = url.baseDomain else {
            self.backgroundColor = .gray
            completionBlock?(UIColor.Photon.Grey50, url)
            return
        }

        if let color = FaviconFetcher.colors[domain] {
            self.backgroundColor = color
            completionBlock?(color, url)
        } else {
            image.getColors(scaleDownSize: CGSize(width: 25, height: 25)) {colors in
                let isSame = [colors.primary, colors.secondary, colors.detail].every { $0 == colors.primary }
                if isSame {
                    completionBlock?(UIColor.Photon.White100, url)
                    FaviconFetcher.colors[domain] = UIColor.Photon.White100
                } else {
                    completionBlock?(colors.background, url)
                    FaviconFetcher.colors[domain] = colors.background
                }
            }
        }
    }

    public func setFavicon(forSite site: Site, onCompletion completionBlock: ((UIColor, URL?) -> Void)? = nil ) {
        self.setIcon(site.icon, forURL: site.tileURL, completed: completionBlock)
    }

    private func defaultFavicon(_ url: URL?) -> (image: UIImage, color: UIColor) {
        if let url = url {
            return (FaviconFetcher.getDefaultFavicon(url), FaviconFetcher.getDefaultColor(url))
        } else {
            return (FaviconFetcher.defaultFavicon, .white)
        }
    }
}

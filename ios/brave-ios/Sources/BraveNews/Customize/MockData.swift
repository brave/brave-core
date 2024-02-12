// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

#if DEBUG
enum Mock {
  static var sources: [FeedItem.Source] {
    let json = """
[{
  "background_color": "#004b7d",
  "category": "Technology",
  "cover_url": "https://pcdn.bravesoftware.com/brave-today/cover_images/886ba84a02f823342663a3861cf8ba0a4d1e255a9f70984baf4c767684dcb806.jpg.pad",
  "destination_domains": [
    "9to5mac.com",
    "9to5toys.com"
  ],
  "enabled": false,
  "favicon_url": "https://9to5mac.com/favicon.ico",
  "feed_url": "https://9to5mac.com/feed/",
  "locales": [
    {
      "channels": [
        "Technology"
      ],
      "locale": "en_CA",
      "rank": 150
    },
    {
      "channels": [
        "Technology"
      ],
      "locale": "en_US",
      "rank": 167
    }
  ],
  "publisher_id": "0da7a01841198f6ae17eb5ab6107f5989b7561eb96d9009d1e10d152bb72db25",
  "publisher_name": "9to5Mac",
  "score": 0.0,
  "site_url": "https://9to5mac.com"
},
  {
    "background_color": "#ffffff",
    "category": "Top News",
    "cover_url": "https://abc.es/favicon-192x192.png",
    "destination_domains": [
      "www.abc.es"
    ],
    "enabled": false,
    "favicon_url": "https://s1.abcstatics.com/comun/2018/img/iconos-metas/favicon-2016.ico",
    "feed_url": "https://www.abc.es/rss/feeds/abc_ultima.xml",
    "locales": [
      {
        "channels": [
          "Top News"
        ],
        "locale": "es_MX",
        "rank": 13
      }
    ],
    "publisher_id": "8126fdaf01cc2a8bd6b345e4daedc85b2eda0d05e6bc59fc53fced990878ffa7",
    "publisher_name": "ABC",
    "score": 0.0,
    "site_url": "https://abc.es"
  }]
"""
    // swiftlint:disable:next force_try
    return try! JSONDecoder().decode([FeedItem.Source].self, from: json.data(using: .utf8)!)
  }
  
  static var channels: [String] {
    [
      "Brave",
      "Business",
      "Cars",
      "Crypto",
      "Culture",
      "Entertainment",
      "Fashion",
      "Food",
      "Fun",
      "Gaming",
      "Health",
      "Home",
      "Politics",
      "Science",
      "Sports",
      "Technology",
      "Top News",
      "Top Sources",
      "Travel",
      "Weather",
      "World News",
    ]
  }
}
#endif

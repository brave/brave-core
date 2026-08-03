// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

export const BackArrow = <Icon name='arrow-small-left' />

export const ArrowRight = <Icon name='arrow-small-right' />

export const channelIcons: { [category: string]: JSX.Element } = {
  'default': <Icon name='product-brave-news' />,
  'Brave': <Icon name='brave-icon-only-face' />,
  'Business': <Icon name='news-business' />,
  'Cars': <Icon name='news-car' />,
  'Crypto': <Icon name='crypto-wallets' />,
  'Culture': <Icon name='news-culture' />,
  'Entertainment': <Icon name='news-entertainment' />,
  'Entertainment News': <Icon name='news-entertainment' />,
  'Fashion': <Icon name='news-fashion' />,
  'Film and TV': <Icon name='news-filmandtv' />,
  'Food': <Icon name='news-food' />,
  'Fun': <Icon name='news-fun' />,
  'Gaming': <Icon name='news-gaming' />,
  'Health': <Icon name='news-health' />,
  'Home': <Icon name='news-home' />,
  'Music': <Icon name='news-music' />,
  'Politics': <Icon name='news-politics' />,
  'Regional News': <Icon name='news-regional' />,
  'Science': <Icon name='news-science' />,
  'Sports': <Icon name='news-sports' />,
  'Travel': <Icon name='news-travel' />,
  'Technology': <Icon name='news-technology' />,
  'Tech News': <Icon name='news-technology' />,
  'Tech Reviews': <Icon name='news-technology' />,
  'Top News': <Icon name='news-topnews' />,
  'US News': <Icon name='news-regional' />,
  'Weather': <Icon name='news-weather' />,
  'World News': <Icon name='news-worldnews' />
}

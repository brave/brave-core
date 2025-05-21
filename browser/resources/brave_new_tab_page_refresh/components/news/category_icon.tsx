/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

interface Props {
  category: string
}

export function CategoryIcon(props: Props) {
  switch (props.category) {
    case 'Brave':
      return <Icon name='brave-icon-monochrome' />
    case 'Business':
      return <Icon name='news-business' />
    case 'Cars':
      return <Icon name='news-car' />
    case 'Crypto':
      return <Icon name='crypto-wallets' />
    case 'Culture':
      return <Icon name='news-culture' />
    case 'Entertainment':
      return <Icon name='news-entertainment' />
    case 'Entertainment News':
      return <Icon name='news-entertainment' />
    case 'Fashion':
      return <Icon name='news-fashion' />
    case 'Film and TV':
      return <Icon name='news-filmandtv' />
    case 'Food':
      return <Icon name='news-food' />
    case 'Fun':
      return <Icon name='news-fun' />
    case 'Gaming':
      return <Icon name='news-gaming' />
    case 'Health':
      return <Icon name='news-health' />
    case 'Home':
      return <Icon name='news-home' />
    case 'Music':
      return <Icon name='news-music' />
    case 'Politics':
      return <Icon name='news-politics' />
    case 'Regional News':
      return <Icon name='news-regional' />
    case 'Science':
      return <Icon name='news-science' />
    case 'Sports':
      return <Icon name='news-sports' />
    case 'Travel':
      return <Icon name='news-travel' />
    case 'Technology':
      return <Icon name='news-technology' />
    case 'Tech News':
      return <Icon name='news-technology' />
    case 'Tech Reviews':
      return <Icon name='news-technology' />
    case 'Top News':
      return <Icon name='news-topnews' />
    case 'US News':
      return <Icon name='news-regional' />
    case 'Weather':
      return <Icon name='news-weather' />
    case 'World News':
      return <Icon name='news-worldnews' />
    default:
      return <Icon name='product-brave-news' />
  }
}

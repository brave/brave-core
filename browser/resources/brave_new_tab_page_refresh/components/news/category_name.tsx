/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StringKey, getString } from '../../lib/strings'

interface Props {
  category: string
}

export function CategoryName(props: Props) {
  const key = getCategoryStringKey(props.category)
  if (key) {
    return <>{getString(key)}</>
  }
  return <>{props.category}</>
}

function getCategoryStringKey(category: string): StringKey | null {
  switch (category) {
    case 'Brave':
      return S.BRAVE_NEWS_CHANNEL_BRAVE
    case 'Business':
      return S.BRAVE_NEWS_CHANNEL_BUSINESS
    case 'Cars':
      return S.BRAVE_NEWS_CHANNEL_CARS
    case 'Celebrities':
      return S.BRAVE_NEWS_CHANNEL_CELEBRITIES
    case 'Crypto':
      return S.BRAVE_NEWS_CHANNEL_CRYPTO
    case 'Culture':
      return S.BRAVE_NEWS_CHANNEL_CULTURE
    case 'Education':
      return S.BRAVE_NEWS_CHANNEL_EDUCATION
    case 'Entertainment':
      return S.BRAVE_NEWS_CHANNEL_ENTERTAINMENT
    case 'Fashion':
      return S.BRAVE_NEWS_CHANNEL_FASHION
    case 'Film and TV':
      return S.BRAVE_NEWS_CHANNEL_FILM_AND_TV
    case 'Food':
      return S.BRAVE_NEWS_CHANNEL_FOOD
    case 'Fun':
      return S.BRAVE_NEWS_CHANNEL_FUN
    case 'Gaming':
      return S.BRAVE_NEWS_CHANNEL_GAMING
    case 'Health':
      return S.BRAVE_NEWS_CHANNEL_HEALTH
    case 'Home':
      return S.BRAVE_NEWS_CHANNEL_HOME
    case 'Lifestyle':
      return S.BRAVE_NEWS_CHANNEL_LIFESTYLE
    case 'Music':
      return S.BRAVE_NEWS_CHANNEL_MUSIC
    case 'Politics':
      return S.BRAVE_NEWS_CHANNEL_POLITICS
    case 'RegionalNews':
      return S.BRAVE_NEWS_CHANNEL_REGIONAL_NEWS
    case 'Science':
      return S.BRAVE_NEWS_CHANNEL_SCIENCE
    case 'Sports':
      return S.BRAVE_NEWS_CHANNEL_SPORTS
    case 'Travel':
      return S.BRAVE_NEWS_CHANNEL_TRAVEL
    case 'Technology':
      return S.BRAVE_NEWS_CHANNEL_TECHNOLOGY
    case 'Top News':
      return S.BRAVE_NEWS_CHANNEL_TOP_NEWS
    case 'Top Sources':
      return S.BRAVE_NEWS_CHANNEL_TOP_SOURCES
    case 'UK News':
      return S.BRAVE_NEWS_CHANNEL_UK_NEWS
    case 'US News':
      return S.BRAVE_NEWS_CHANNEL_US_NEWS
    case 'Weather':
      return S.BRAVE_NEWS_CHANNEL_WEATHER
    case 'World News':
      return S.BRAVE_NEWS_CHANNEL_WORLD_NEWS
    default:
      return null
  }
}

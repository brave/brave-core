/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StringKey } from '../../models/locale_strings'
import { useLocale } from '../context/locale_context'

interface Props {
  category: string
}

export function CategoryName(props: Props) {
  const { getString } = useLocale()
  const key = getCategoryStringKey(props.category)
  if (key) {
    return <>{getString(key)}</>
  }
  return <>{props.category}</>
}

function getCategoryStringKey(category: string): StringKey | null {
  switch (category) {
    case 'Brave': return 'newsChannelBrave'
    case 'Business': return 'newsChannelBusiness'
    case 'Cars': return 'newsChannelCars'
    case 'Celebrities': return 'newsChannelCelebrities'
    case 'Crypto': return 'newsChannelCrypto'
    case 'Culture': return 'newsChannelCulture'
    case 'Education': return 'newsChannelEducation'
    case 'Entertainment': return 'newsChannelEntertainment'
    case 'Fashion': return 'newsChannelFashion'
    case 'Film and TV': return 'newsChannelFilmAndTV'
    case 'Food': return 'newsChannelFood'
    case 'Fun': return 'newsChannelFun'
    case 'Gaming': return 'newsChannelGaming'
    case 'Health': return 'newsChannelHealth'
    case 'Home': return 'newsChannelHome'
    case 'Lifestyle': return 'newsChannelLifestyle'
    case 'Music': return 'newsChannelMusic'
    case 'Politics': return 'newsChannelPolitics'
    case 'RegionalNews': return 'newsChannelRegionalNews'
    case 'Science': return 'newsChannelScience'
    case 'Sports': return 'newsChannelSports'
    case 'Travel': return 'newsChannelTravel'
    case 'Technology': return 'newsChannelTechnology'
    case 'Top News': return 'newsChannelTopNews'
    case 'Top Sources': return 'newsChannelTopSources'
    case 'UK News': return 'newsChannelUKNews'
    case 'US News': return 'newsChannelUSNews'
    case 'Weather': return 'newsChannelWeather'
    case 'World News': return 'newsChannelWorldNews'
    default: return null
  }
}

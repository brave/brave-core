/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useShieldsApi } from '../api/shields_api_context'

import { style } from './details_header.style'

interface Props {
  title: string
  onBack: () => void
  children?: React.ReactNode
}

export function DetailsHeader(props: Props) {
  const api = useShieldsApi()
  const { getSiteBlockInfoData } = api.useGetSiteBlockInfo()
  const host = getSiteBlockInfoData?.host ?? ''

  return (
    <div data-css-scope={style.scope}>
      <div className='header'>
        <button onClick={props.onBack}>
          <Icon name='arrow-left' />
        </button>
        <div className='text'>
          <h4>{props.title}</h4>
          <div className='host'>{host}</div>
        </div>
      </div>
      {props.children}
    </div>
  )
}

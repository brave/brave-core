// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { ArrowUpIcon, LoaderIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../../common/locale'

type Props = {
  show: boolean
  isFetching: boolean
  onClick: () => any
}

const RefreshButton = styled('button')<Props>`
  appearance: none;
  position: fixed;
  top: 33px;
  left: 50%;
  transform: translateX(-50%);
  pointer-events: ${p => p.show ? 'all' : 'none'};
  opacity: ${p => p.show ? 1 : 0};
  outline: none;
  border: none;
  border-radius: 24px;
  background: ${p => p.theme.color.brandBat};
  padding: 12px 30px;
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 10px;
  cursor: pointer;
  font-weight: 800;
  font-size: 16px;
  color: white;
  transition: opacity 1s ease-in-out, background .124s ease-in-out;
  &:hover {
    background: ${p => p.theme.color.brandBatInteracting};
  }
  &:active {
    background: ${p => p.theme.color.brandBatActive};
  }
  &:focus-visible {
    box-shadow: 0 0 0 1px white;
  }
`

const Icon = styled('div')`
  width: 16px;
  height: 16px;
`

export default function Refresh (props: Props) {
  const text = props.isFetching
    ? getLocale('braveTodayStatusFetching')
    : getLocale('braveTodayActionRefresh')
  const graphic = props.isFetching
    ? <LoaderIcon />
    : <ArrowUpIcon />
  return (
    <RefreshButton {...props}>
      <Icon>
        {graphic}
      </Icon>
      <span>{text}</span>
    </RefreshButton>
  )
}

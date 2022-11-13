// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import InfoIconSvg from './info-icon.svg'
import { makeSizeMixin } from '../../../utils/style.utils'
import LoadingIcon from '../../../assets/svg-icons/loading-slow.svg'

export const BoxRow = styled.div<{ paddingTop?: number; paddingBottom?: number }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: -webkit-fill-available;
  height: inherit;
  padding-left: 12px;
  padding-right: 12px;
  padding-top: ${p => p.paddingTop || 0}px;
  padding-bottom: ${p => p.paddingBottom || 0}px;
`

export const BoxColumn = styled.div<{ color: 'text02' | 'errorIcon' | 'successIcon' }>`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  color: ${p => p.theme.color[p.color]};
`

export const Box = styled.div<{ isDetail: boolean }>`
  display: flex;
  align-items: flex-start;
  justify-content: 'flex-start';
  flex-direction: column;
  border: 1px solid ${p => p.theme.color.divider01};
  box-sizing: border-box;
  box-shadow: 0 0 1px rgba(66, 69, 82, 0.08), 0 0.5px 1.5px rgba(66, 69, 82, 0.1);
  border-radius: 8px;
  width: calc(100% - 8px);
  max-height: 186px;
  min-height: 186px;
  margin-bottom: 8px;
  overflow-y: scroll;
  overflow-x: hidden;
  position: relative;

  ${p =>
    p.isDetail
      ? `
    padding: 8px;
  `
      : 'padding-bottom: 8px;'}
`

export const BoxTitle = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 18px;
  color: ${p => p.theme.color.text01};
`

export const InfoCircleIcon = styled.div<{
  size?: number | string
}>`
  ${makeSizeMixin(16)}
  background-color: #6B7084; // Not a theme color
  -webkit-mask-image: url(${InfoIconSvg});
  mask-image: url(${InfoIconSvg});
  mask-size: contain;
  mask-position: center;
  mask-repeat: no-repeat;
`

export const Divider = styled.div`
  border: 1px solid ${p => p.theme.color.divider01};
  height: 0;
  width: 100%;
`

export const BoxSubTitle = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 18px;
  color: ${p => p.theme.color.text03};
`

export const Loader = styled.div`
  background: url(${LoadingIcon});
  width: 100%;
  height: 100%;
  opacity: 0.4;
  background-repeat: no-repeat;
  margin-left: 60px; ;
`

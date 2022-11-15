// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// icon images
import HistoryIcon from '../../../assets/svg-icons/history-icon.svg'
import WarningCircle from '../../../assets/svg-icons/warning-circle-icon.svg'

// css mixins
import { makeSizeMixin, sizeCssValue } from '../../../utils/style.utils'
import { ThemeColor } from '../../shared/style'

export const StyledWrapper = styled.div<{ hideScrollbar?: boolean }>`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  overflow: ${(p) => p?.hideScrollbar
    ? 'hidden'
    : 'unset'
  };
`

export const FloatAboveBottomRightCorner = styled.div`
  position: absolute;
  bottom: -8%;
  right: -8%;
`

export const FillerTitleText = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 16px;
  line-height: 24px;
  text-align: center;
  color: ${(p) => p.theme.color.text01};
  margin: 0px;
`

export const FillerDescriptionText = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  margin: 0px;
`

export const TransactionsIcon = styled.div<{
  color?: ThemeColor
  size?: number | string
}>`
  ${makeSizeMixin(18)}
  -webkit-mask-image: url(${HistoryIcon});
  mask-image: url(${HistoryIcon});
  mask-size: ${(p) => sizeCssValue(p?.size || '18px')};
  mask-position: center;
  mask-repeat: no-repeat;
  background-color: #656565; /* Not a theme color */
  @media (prefers-color-scheme: dark) {
    background-color: #C2C4CF; /* Not a theme color */
  }
`

export const InfoCircleIcon = styled.div<{
  size?: number | string
}>`
  ${makeSizeMixin(18)}
  background-color: #6B7084; // Not a theme color
  -webkit-mask-image: url(${WarningCircle});
  mask-image: url(${WarningCircle});
  mask-size: contain;
  mask-position: center;
  mask-repeat: no-repeat;
`

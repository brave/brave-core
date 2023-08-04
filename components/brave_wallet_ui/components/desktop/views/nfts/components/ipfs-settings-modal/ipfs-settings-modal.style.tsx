// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import { Column } from '../../../../../shared/style'
import { layoutPanelWidth } from '../../../../wallet-page-wrapper/wallet-page-wrapper.style'

export const ContentWrapper = styled(Column)`
  padding: 16px 20px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px;
  }
`

export const IpfsIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  min-width: 40px;
  height: 40px;
  background-color: ${leo.color.container.highlight};
  border-radius: 100px;
`

export const IpfsIcon = styled(Icon).attrs({
  name: 'product-ipfs-outline'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
  background-color: transparent;
`

export const SettingHeader = styled.h2`
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 600;
  line-height: 24px;
  padding: 0;
  margin: 0;
  color: ${leo.color.text.primary};
  width: 100%;
  text-align: left;
`

export const SettingDescription = styled.p`
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 400;
  line-height: 18px;
  color: ${leo.color.text.secondary};
  margin: 0;
  padding: 0;
`

export const SectionHeader = styled.p`
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 600;
  line-height: 24px;
  margin: 0;
  padding: 0;
  color: ${leo.color.text.primary};
`

export const ProgressCount = styled.p`
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 500;
  line-height: 18px;
  margin: 0;
  padding: 0;
  color: ${leo.color.text.secondary};
`

export const CaratIcon = styled(Icon)`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`

export const Divider = styled.div`
  display: flex;
  width: 100%;
  padding: 0px 8px;
  flex-direction: column;
  justify-content: center;
  align-items: flex-start;
  border-bottom: 1px solid ${leo.color.divider.subtle};
`

export const CollapseIcon = styled(Icon) <
  {
    isCollapsed: boolean
  }>`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
  transition-duration: 0.3s;
  transform: ${(p) =>
    p.isCollapsed
      ? 'unset'
      : 'rotate(180deg)'
  };
  margin-left: 16px;
`

export const NftGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  grid-gap: 24px;
  box-sizing: border-box;
  width: 100%;
  @media screen and (max-width: 700px) {
    grid-template-columns: repeat(2, 1fr);
  }
  @media screen and (max-width: 545px) {
    grid-template-columns: repeat(1, 1fr);
  }
  padding-top: 32px;
`

export const GridItem = styled.div`
  position: relative;
  width: 132px;
  height: 132px;
`
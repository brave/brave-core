// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import {
  layoutPanelWidth,
  layoutSmallWidth
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { WalletButton } from '../../../components/shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: flex-start;
  width: 100%;
  padding: 16px 12px 12px 12px;
`

export const TopWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  align-self: center;
  justify-content: flex-start;
  width: 360px;
  text-align: left;
  margin-bottom: 8px;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    min-width: 100%;
  }
`

export const NftName = styled.h2`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 16px;
  line-height: 28px;
  text-align: left;
  color: ${leo.color.text.primary};
  width: 100%;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  margin: 0;
  padding: 16px 0 0 0;
`

export const CollectionName = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  text-align: left;
  color: ${leo.color.legacy.text3};
  width: 100%;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  margin: 0;
  padding: 0;
`

export const PurchaseDate = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  text-align: left;
  color: ${leo.color.legacy.text3};
  width: 100%;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  margin: 0;
  padding: 0;
`

export const SectionTitle = styled.h3`
  display: flex;
  flex-direction: row;
  align-items: center;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 16px;
  line-height: 24px;
  text-align: left;
  color: ${leo.color.text.primary};
  margin: 0;
  padding: 16px 0;
`

export const SectionWrapper = styled.div`
  display: grid;
  grid-auto-flow: column;
  grid-auto-columns: minmax(0, 1fr);
  width: 100%;
  gap: 16px;
  margin-bottom: 16px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    grid-auto-flow: row;
  }
`

export const InfoBox = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  padding: 8px 16px;
  background: ${leo.color.container.highlight};
  border-radius: 8px;
  min-width: 0;
  position: relative;
`

export const InfoTitle = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${leo.color.text.secondary};
  margin: 0;
  padding: 0;
  width: 100%;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
`

export const InfoText = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
  margin: 0;
  padding: 0;
  width: 100%;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
`

export const AccountName = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const AccountAddress = styled(AccountName)`
  color: ${leo.color.text.secondary};
`

export const CopyIcon = styled(Icon)`
  --leo-icon-size: 13px;
  color: ${leo.color.text.tertiary};
  cursor: pointer;

  &:hover {
    color: ${leo.color.text.interactive};
  }
`

export const ViewAccount = styled(WalletButton)`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 16px;
  display: flex;
  align-items: center;
  text-align: center;
  align-self: flex-end;
  letter-spacing: 0.03em;
  color: ${leo.color.text.interactive};
  border: transparent;
  background-color: transparent;
  cursor: pointer;
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
`

export const Description = styled.p`
  width: 100%;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
  margin: 0;
  padding: 0;
`

export const HighlightedButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 8px;
  border: none;
  background-color: transparent;
  overflow: hidden;
  padding: 0;
  max-width: 100%;
  cursor: pointer;
`

export const ImageLink = styled.a`
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 8px;
  border: none;
  background-color: transparent;
  overflow: hidden;
  padding: 0;
  max-width: 100%;
  cursor: pointer;
  text-decoration: none;
`

export const HighlightedText = styled(InfoText)`
  color: ${leo.color.text.interactive};
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 16px;
  color: ${leo.color.text.interactive};
`

export const Divider = styled.div`
  width: 100%;
  border-bottom: 1px solid ${leo.color.divider.subtle};
`

export const Properties = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  padding: 8px 16px;
  gap: 8px;
  width: 100%;
  background: ${leo.color.container.highlight};
  border-radius: 8px;
  flex: none;
  order: 0;
  flex-grow: 1;
`

export const Property = styled.div`
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  width: 100%;
`

export const Trait = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: flex-start;
`

export const TraitType = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${leo.color.text.secondary};
  overflow: hidden;
  -webkit-line-clamp: 1;
  text-overflow: ellipsis;
`

export const TraitValue = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
  overflow: hidden;
  -webkit-line-clamp: 1;
  text-overflow: ellipsis;
`

export const TraitRarity = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
  overflow: hidden;
  -webkit-line-clamp: 1;
  text-overflow: ellipsis;
`

export const InfoIcon = styled(Icon)`
  --leo-icon-size: 20px;
  color: ${leo.color.systemfeedback.errorIcon};
`

export const ErrorWrapper = styled.div`
  position: relative;
`

export const ErrorMessage = styled.p`
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-family: Poppins;
  letter-spacing: 0.01em;
  font-size: 11px;
  line-height: 16px;
  color: ${(p) => p.theme.color.text02};
  word-break: break-word;
  margin: 24px 0;
`

export const NftMultimediaWrapper = styled.div`
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-end;
  width: 100%;
`

export const IconWrapper = styled.div`
  position: absolute;
  display: flex;
  justify-content: flex-end;
  width: 360px;
  z-index: 3;
`

export const NetworkIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: absolute;
  bottom: -12px;
  right: -1px;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
  padding: 2px;
`

export const NftMultimedia = styled.iframe<{ visible?: boolean }>`
  width: 100%;
  height: ${(p) => (p.visible ? '360px' : '0px')};
  border: none;
  visibility: ${(p) => (p.visible ? 'visible' : 'hidden')};
`

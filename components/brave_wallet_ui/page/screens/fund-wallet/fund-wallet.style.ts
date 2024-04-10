// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Column } from '../../../components/shared/style'

export const SelectAssetWrapper = styled(Column)`
  box-sizing: border-box;
  overflow: hidden;
`

export const StyledWrapper = styled(SelectAssetWrapper)`
  flex: 1;
`

export const ScrollContainer = styled.div`
  width: 100%;
  display: flex;
  flex-direction: column;
  overflow-y: auto;
`

export const TokenListWrapper = styled(StyledWrapper)`
  overflow-y: auto;
`

export const SearchWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: column;
  width: 100%;
  position: relative;
`

export const QRCodeContainer = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 260px;
  height: 260px;
  border-radius: 4px;
  border: 4px solid ${(p) => p.theme.color.text01};
  margin-bottom: 16px;
`

export const QRCodeImage = styled.img`
  width: 260px;
  height: 260px;
`

export const AddressText = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 14px;
  line-height: 20px;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  letter-spacing: 0.01em;
  word-break: break-all;
`
export const AddressTextLabel = styled(AddressText)`
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
`

export const Title = styled.p`
  font-family: Poppins;
  font-size: 14px;
  line-height: 24px;
  font-weight: 600;
  color: ${leo.color.text.primary};
  margin: 0px;
  text-align: left;
  align-self: flex-start;
`

export const Description = styled.p`
  font-family: Poppins;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.secondary};
  text-align: left;
  align-self: flex-start;
  margin: 0;
`

export const Divider = styled.div`
  width: 100%;
  border-bottom: 1px solid ${leo.color.divider.subtle};
`

export const Alert = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 16px;
  padding: 16px;
  border-radius: 16px;
  margin-top: 16px;
  background-color: ${leo.color.systemfeedback.infoBackground};
`

export const InfoIcon = styled(Icon)`
  --leo-icon-size: 20px;
  color: ${leo.color.systemfeedback.infoIcon};
`

export const AlertText = styled.p`
  font-size: 14px;
  font-family: Poppins;
  line-height: 24px;
  color: ${leo.color.text.primary};
  margin: 0;
  padding: 0;
`

export const SearchAndDropdownWrapper = styled(Column)`
  flex: 1;
  min-width: 25%;
`

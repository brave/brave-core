// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import LeoSegmentedControl, {
  SegmentedControlProps,
} from '@brave/leo/react/segmentedControl'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Title } from '../onboarding/onboarding.style'
import { Row, Text, Column } from '../../../components/shared/style'

export const DepositTitle = styled(Title)`
  margin-top: 0px;
`

export const ControlsWrapper = styled(Row)`
  margin-bottom: 24px;
  --leo-segmented-control-width: 100%;
`

export const SegmentedControl = styled(LeoSegmentedControl).attrs({
  size: 'default',
})<SegmentedControlProps>``

export const AddressText = styled(Text)`
  word-break: break-all;
  max-width: 500px;
`

export const AddressTextLabel = styled(Text)`
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
  font-family: 'Poppins';
  font-style: normal;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  word-break: break-all;
`

export const TokenListWrapper = styled(Column)`
  box-sizing: border-box;
  overflow: hidden;
  flex: 1;
  overflow-y: auto;
`

export const QRCodeContainer = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 260px;
  height: 260px;
  border-radius: 4px;
  border: 4px solid ${leo.color.text.primary};
  margin-bottom: 16px;
`

export const QRCodeImage = styled.img`
  width: 260px;
  height: 260px;
`

export const ScrollContainer = styled.div`
  width: 100%;
  display: flex;
  flex-direction: column;
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

export const SelectAssetWrapper = styled(Column)`
  box-sizing: border-box;
  overflow: hidden;
`

export const SearchAndDropdownWrapper = styled(Column)`
  flex: 1;
  min-width: 25%;
`

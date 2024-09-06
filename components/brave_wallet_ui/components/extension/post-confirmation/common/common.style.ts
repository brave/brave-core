// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import ProgressRing from '@brave/leo/react/progressRing'
import LeoIcon from '@brave/leo/react/icon'
import LeoAlert from '@brave/leo/react/alert'
import * as leo from '@brave/leo/tokens/css/variables'
import LinkSvg from '../../../../assets/svg-icons/link-icon.svg'
import LoadingIcon from '../../../../assets/svg-icons/loading-slow.svg'
import { WalletButton, Column, Text } from '../../../shared/style'

export const Wrapper = styled(Column)`
  background-color: ${leo.color.container.highlight};
`

export const Title = styled(Text)`
  font: ${leo.font.heading.h3};
  margin-bottom: 8px;
`

export const TransactionStatusDescription = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 20px;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  padding: 8px 16px;
`

export const LinkIcon = styled.div`
  width: 12px;
  height: 12px;
  background: url(${LinkSvg});
  margin-left: 8px;
`

export const DetailButton = styled(WalletButton)`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 20px;
  text-align: center;
  color: ${(p) => p.theme.color.interactive05};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0;
  padding: 0;
`

export const Loader = styled.div`
  background: url(${LoadingIcon});
  width: 220px;
  height: 220px;
  margin: 36px 0;
  opacity: 0.4;
`

export const LoadingRing = styled(ProgressRing)`
  --leo-progressring-size: 110px;
  --leo-progressring-color: ${leo.color.icon.interactive};
  margin-bottom: 46px;
`

export const StatusIcon = styled(LeoIcon)`
  --leo-icon-size: 34px;
  color: ${leo.color.icon.interactive};
`

export const Button = styled(WalletButton)`
  cursor: pointer;
  background-color: none;
  background: none;
  outline: none;
  border: none;
  padding: 0px;
  margin: 0px;
`

export const HeaderButton = styled(Button)`
  margin: 2px 0px;
`

export const HeaderIcon = styled(LeoIcon)`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const ExplorerIcon = styled(LeoIcon).attrs({
  name: 'arrow-diagonal-up-right'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.interactive};
`

export const Alert = styled(LeoAlert)`
  width: 100%;
  --leo-alert-padding: 12px;
  margin-bottom: 46px;
`

export const ErrorOrSuccessIconWrapper = styled.div<{
  kind: 'error' | 'success'
}>`
  padding: 20px;
  border-radius: 100%;
  background-color: ${(p) =>
    p.kind === 'error'
      ? leo.color.systemfeedback.errorBackground
      : leo.color.systemfeedback.successBackground};
  margin-bottom: 40px;
`

export const ErrorOrSuccessIcon = styled(LeoIcon)<{
  kind: 'error' | 'success'
}>`
  --leo-icon-size: 60px;
  color: ${(p) =>
    p.kind === 'error'
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.successIcon};
`

// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

// types
import { BraveWallet } from '../../../constants/types'

// Shared styles
import { WalletButton } from '../../shared/style'

// Assets
import Upload from '../../../assets/svg-icons/nft-ipfs/upload.svg'
import Check from '../../../assets/svg-icons/nft-ipfs/check.svg'
import Close from '../../../assets/svg-icons/close.svg'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  gap: 6px;
  position: relative;
`

export const ContentWrapper = styled.div<{
  pinningStatus: BraveWallet.TokenPinStatusCode
}>`
  --success-background: ${leo.color.systemfeedback.successBackground};
  --uploading-background: ${leo.color.systemfeedback.warningBackground};
  --failed-background: ${leo.color.systemfeedback.errorBackground};

  @media (prefers-color-scheme: dark) {
    --failed-background: rgba(227, 36, 68, 0.2);
  }
  display: flex;
  justify-content: flex-start;
  align-items: center;
  flex-shrink: 0;
  gap: 4px;
  border-radius: 4px;
  padding: 2px 4px;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.text02};
  background-color: ${(p) =>
    (p.pinningStatus === BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS) ||
    (p.pinningStatus === BraveWallet.TokenPinStatusCode.STATUS_PINNING_PENDING)

      ? 'var(--uploading-background)'
      : p.pinningStatus === BraveWallet.TokenPinStatusCode.STATUS_PINNED
      ? 'var(--success-background)'
      : 'var(--failed-background)'};
`

export const StatusIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 10px;
  height: 10px;
  mask-repeat: no-repeat;
`

export const UploadIcon = styled(StatusIcon)`
  mask-image: url(${Upload});
  -webkit-mask-image: url(${Upload});
  background-color: #febf17;
`

export const CloseIcon = styled(StatusIcon)`
  mask-image: url(${Close});
  -webkit-mask-image: url(${Close});
  background-color: #e32444;
`

export const CheckIcon = styled(StatusIcon)`
  width: 14px;
  height: 14px;
  mask-image: url(${Check});
  -webkit-mask-image: url(${Check});
  background-color: #4dc661;
`

export const ReportButton = styled(WalletButton)`
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  border: none;
  color: #4c54d2;
  outline: none;
  border: none;
  background: transparent;
`

export const Text = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.text02};
`

export const ReasonsTooltipWrapper = styled.div`
  position: absolute;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  z-index: 3;
  background-color: ${(p) => p.theme.color.background01};
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  width: 246px;
  border-radius: 6px;
  bottom: 36px;
  left: 0;
`

export const TooltipContent = styled.div`
  position: relative;
  padding: 8px 16px 12px;
  width: 100%;
  height: 100%;
`

export const TooltipHeading = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
`

export const TooltipList = styled.ul`
  list-style-position: inside;
  margin: 8px 0 0 0;
  padding: 0;

  li {
    font-family: 'Poppins';
    font-style: normal;
    font-weight: 400;
    font-size: 12px;
    line-height: 18px;
    padding: 0;
    margin: 0;
    color: ${(p) => p.theme.color.text01};
  }
`

export const ArrowDown = styled.div`
  width: 0;
  height: 0;
  border-left: 8px solid transparent;
  border-right: 8px solid transparent;
  border-top: 8px solid ${(p) => p.theme.color.background01};
  position: absolute;
  left: 56px;
  bottom: -8px;
`

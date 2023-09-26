// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import { WalletButton } from '../../../shared/style'
import { layoutPanelWidth } from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 0 32px 32px 32px;
  border-radius: 16px;
  z-index: 2;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0 24x 24px 24px;
  }
`

export const Header = styled.div`
  color: ${leo.color.text.primary};
  text-align: center;
  font-family: Poppins;
  font-size: 22px;
  font-style: normal;
  font-weight: 500;
  line-height: 32px;
  text-align: left;
  width: 100%;
  padding-bottom: 16px;
`

export const Description = styled.div`
  color: ${leo.color.text.secondary};
  text-align: center;
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
  line-height: 24px;
  padding-bottom: 24px;
`

export const ButtonRow = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  width: 100%;
  gap: ${leo.spacing.xl};

  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column-reverse;
    justify-content: center;
    gap: ${leo.spacing.l};
  }
`

export const ConfirmButton = styled(WalletButton)`
  display: flex;
  justify-content: center;
  align-items: center;
  flex: 1 0 0;
  padding: 12px 16px;
  background-color: ${leo.color.button.background};
  border-radius: 1000px;
  font-family: Poppins;
  font-size: 13px;
  font-style: normal;
  font-weight: 600;
  line-height: 20px;
  color: ${leo.color.white};
  border: none;
  cursor: pointer;
  align-self: stretch;
`

export const CancelButton = styled(WalletButton)`
  display: flex;
  justify-content: center;
  flex: 1 0 0;
  align-items: center;
  padding: 12px 16px;
  background-color: transparent;
  border-radius: 1000px;
  font-family: Poppins;
  font-size: 13px;
  font-style: normal;
  font-weight: 600;
  line-height: 20px;
  color: ${leo.color.text.interactive};
  border: none;
  cursor: pointer;
  border: 1px solid ${leo.color.divider.interactive};
  align-self: stretch;
`

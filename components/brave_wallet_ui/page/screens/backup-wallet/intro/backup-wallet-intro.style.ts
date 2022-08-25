// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// icons
import BackupIcon from '../../../../assets/svg-icons/onboarding/backup-your-crypto.svg'
import BackupIconDark from '../../../../assets/svg-icons/onboarding/backup-your-crypto-dark.svg'

// styles
import { WalletButton } from '../../../../components/shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding-top: 32px;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  line-height: 30px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
  margin-bottom: 6px;
`

export const Description = styled.span`
  display: flex;
  align-items: center;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  max-width: 450px;
  text-align: center;
  margin-bottom: 18px;
`

export const TermsRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  max-width: 360px;
  margin-bottom: 30px;
`

export const PasswordColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  max-width: 360px;
  margin-bottom: 30px;
`

export const PageIcon = styled.div`
  width: 200px;
  height: 160px;
  background: url(${BackupIcon});
  background-repeat: no-repeat;
  background-size: 100%;
  margin-bottom: 35px;
  @media (prefers-color-scheme: dark) {
    background: url(${BackupIconDark});
  }
`

export const SkipButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-style: normal;
  font-weight: 500;
  font-size: 13px;
  line-height: 19px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  margin-top: 12px;
`

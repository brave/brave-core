// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { WalletButton } from '../../shared/style'
import icon from '../../../assets/svg-icons/plus-icon.svg'
import { EditOIcon } from 'brave-ui/components/icons'
interface StyleProps {
  buttonType: 'primary' | 'secondary'
}

// Will need to change to brave-ui button

export const StyledButton = styled(WalletButton) <StyleProps>`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  border-radius: 40px;
  padding: 12px 22px;
  outline: none;
  background-color: ${(p) =>
    p.buttonType === 'primary' ? `${p.theme.palette.blurple500}` : 'transparent'};
  border: ${(p) =>
    p.buttonType === 'primary'
      ? 'none'
      : `1px solid ${p.theme.color.interactive08}`};
`

export const ButtonText = styled.span<StyleProps>`
  font-size: 13px;
  font-weight: 600;
  color: ${(p) =>
    p.buttonType === 'primary' ? '#ffffff' : `${p.theme.color.interactive07}`};
`

export const PlusIcon = styled.div`
  width: 15px;
  height: 15px;
  background: url(${icon});
  margin-right: 10px;
`

export const EditIcon = styled(EditOIcon)`
  width: 15px;
  height: 15px;
  color: ${(p) => p.theme.color.interactive07};
  margin-right: 8px;
`

import styled from 'styled-components'
import { SettingsAdvancedIcon } from 'brave-ui/components/icons'

import { WalletButton } from '../../../shared/style'

export const StyledButton = styled(WalletButton)`
  display: flex;
  width: 30px;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  cursor: pointer;
  outline: none;
  padding: 10px 0px 0px 0px;
  border: none;
  background: none;
`

export const SettingsIcon = styled(SettingsAdvancedIcon)`
  padding-bottom: 12px;
`

export const TabLine = styled.div`
  display: flex;
  width: 100%;
  height: 2px;
  background: ${(p) => p.theme.color.divider01};
`

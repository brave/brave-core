// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { WalletButton } from '../shared/style'
import WarningTriangle from '../../assets/svg-icons/warning-triangle.svg'
import IThemeProps from 'brave-ui/src/theme/theme-interface'

interface StyleProps {
  orb: string
  warningType: 'warning' | 'danger'
}

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${(p) => p.theme.color.background01};
`

export const TopRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 15px 15px 0px 15px;
`

export const CenterColumn = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 100%;
`

export const AddressAndOrb = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AccountCircle = styled.div<Partial<StyleProps>>`
  width: 32px;
  height: 32px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
`

export const AddressText = styled.span`
  cursor: default;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  margin-right: 12px;
`

export const NetworkText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const PanelTitle = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
  text-align: center;
  width: 90%;
  color: ${(p) => p.theme.color.text01};
  font-weight: 600;
  margin-bottom: 6px;
`

export const Description = styled.span`
  width: 275px;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 12px;
  width: 60%;
`

export const TabRow = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: row;
  width: 255px;
  margin-bottom: 10px;
`

export const DetailTextDarkBold = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text02};
`

export const DetailTextLight = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const DetailTextDark = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
`

export const URLText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: normal;
  font-size: 10px;
  line-height: 16px;
  text-align: center;
  letter-spacing: 0.01em;
  margin-bottom: 8px;
  color: ${(p) => p.theme.color.text02};
  max-width: 80%;
  word-break: break-word;
`

export const WarningBox = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  box-sizing: border-box;
  border-radius: 4px;
  width: 255px;
  padding: 10px;
  margin-bottom: 14px;
  background-color: ${(p) =>
    p.warningType === 'danger'
      ? p.theme.color.errorBackground
      : p.theme.color.warningBackground
  };
`

export const WarningTitle = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) =>
    p.warningType === 'danger'
      ? p.theme.color.errorText
      : p.theme.color.text01
  };
`

export const WarningBoxTitleRow = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: row;
  flex-wrap: wrap;
`

export const WarningText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: left;
  color: ${(p) => p.theme.color.errorText};
`

export const WarningBoxIcon = styled.div<{
  color?: keyof IThemeProps['color']
}>`
  mask-size: 100%;
  background-color: ${(p) => p?.color
    ? p.theme.color[p.color]
    : p.theme.color.errorIcon
  };
  -webkit-mask-image: url(${WarningTriangle});
  mask-image: url(${WarningTriangle});
`

export const WarningIcon = styled(WarningBoxIcon)`
  width: 18px;
  height: 18px;
  margin-right: 6px;
`

export const LearnMoreButton = styled(WalletButton)`
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0px;
  padding: 0px;
`

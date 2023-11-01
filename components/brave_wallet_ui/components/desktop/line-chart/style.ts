// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled, { css } from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import { LoaderIcon } from 'brave-ui/components/icons'

export const StyledWrapper = styled.div<{
  customStyle?: { [key: string]: string }
}>`
  width: 100%;
  height: 130px;
  min-height: 130px;
  max-height: 130px;
  box-sizing: border-box;
  position: relative;
  ${(p) =>
    p.customStyle
      ? css`
          ${p.customStyle}
        `
      : ''};
`

export const TooltipWrapper = styled.div<{
  labelPosition: 'start' | 'middle' | 'end'
  labelTranslate: number
}>`
  --label-start-translate: translateX(
    calc(-${(p) => p.labelTranslate}px + 4px)
  );
  --label-end-translate: translateX(calc(-100% + ${(p) => p.labelTranslate}px));
  --label-middle-end-condition: ${(p) =>
    p.labelPosition === 'end'
      ? 'var(--label-end-translate)'
      : 'translateX(-50%)'};
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  position: absolute;
  transform: ${(p) =>
    p.labelPosition === 'start'
      ? 'var(--label-start-translate)'
      : 'var(--label-middle-end-condition)'};
  white-space: nowrap;
  background-color: ${leo.color.container.background};
  box-shadow: 0px 4px 16px -1px rgba(0, 0, 0, 0.07);
  border-radius: 4px;
  padding: 2px 4px;
  z-index: 100;
`

export const ChartBalance = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const ChartDate = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 11px;
  line-height: 16px;
  color: ${leo.color.text.secondary};
`

export const AreaWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  position: absolute;
`

export const LoadingOverlay = styled.div<{
  isLoading: boolean
}>`
  display: ${(p) => (p.isLoading ? 'flex' : 'none')};
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  position: absolute;
  backdrop-filter: blur(5px);
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${(p) => p.theme.color.interactive08};
  height: 70px;
  width: 70px;
  opacity: 0.4;
`

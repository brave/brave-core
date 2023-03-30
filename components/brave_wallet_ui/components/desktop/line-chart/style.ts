// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled, { css } from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'

interface StyleProps {
  labelPosition: 'start' | 'middle' | 'end'
  labelTranslate: number
  isLoading: boolean
  customStyle?: { [key: string]: string }
}

export const StyledWrapper = styled.div<Partial<StyleProps>>`
  width: 100%;
  height: 200px;
  min-height: 200px;
  max-height: 200px;
  margin-bottom: 30px;
  box-sizing: border-box;
  position: relative;
  ${p => p.customStyle
    ? css`
      ${p.customStyle}
    `
    : ''
  };
`

export const LabelWrapper = styled.div<Partial<StyleProps>>`
  --label-start-translate: translateX(calc(-${(p) => p.labelTranslate}px + 4px));
  --label-end-translate: translateX(calc(-100% + ${(p) => p.labelTranslate}px));
  --label-middle-end-condition: ${(p) => p.labelPosition === 'end' ? 'var(--label-end-translate)' : 'translateX(-50%)'};
  display: flex;
  align-items: center;
  justify-content: center;
  position: absolute;
  top: -16px;
  transform: ${(p) => p.labelPosition === 'start' ? 'var(--label-start-translate)' : 'var(--label-middle-end-condition)'};
  white-space: nowrap
`

export const ChartLabel = styled.span`
  font-family: Poppins;
  font-size: 13px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
`

export const LoadingOverlay = styled.div<Partial<StyleProps>>`
  display: ${(p) => p.isLoading ? 'flex' : 'none'};
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  position: absolute;
  z-index: 10;
  backdrop-filter: blur(5px);
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 70px;
  width: 70px;
  opacity: .4;
`

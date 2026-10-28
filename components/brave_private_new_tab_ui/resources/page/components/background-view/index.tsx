// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled, { css } from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// The two palettes are distinct literal types, so the parameter only asks
// for the shades the page uses.
type WindowPalette = {
  readonly [shade in 10 | 15 | 20 | 25 | 30 | 60 | 80 | 90]: string
}

function windowPalette (palette: WindowPalette) {
  return css`
    --window-background: ${palette[10]};
    --window-glow: ${palette[15]};
    --window-surface: ${palette[20]};
    --window-surface-strong: ${palette[25]};
    --window-surface-stronger: ${palette[30]};
    --window-accent: ${palette[60]};
    --window-text-secondary: ${palette[80]};
    --window-text-primary: ${palette[90]};
  `
}

// Every component on the page reads its colors from the --window-* variables,
// so Private and Tor windows only differ in the palette set here.
const Box = styled.div<{ isTor?: boolean }>`
  ${p => windowPalette(p.isTor
    ? leo.color.primitive.torWindow
    : leo.color.primitive.privateWindow)}

  display: flex;
  flex-direction: column;
  min-height: 100vh;
  position: relative;
  overflow: hidden;
  background: var(--window-background);
  color: ${leo.color.white};
  font: ${leo.font.default.regular};
`

const Glow = styled.div`
  position: absolute;
  top: -350px;
  left: 50%;
  width: 1024px;
  height: 690px;
  transform: translateX(-50%);
  border-radius: 50%;
  background: var(--window-glow);
  filter: blur(150px);
  pointer-events: none;
`

interface Props {
  children: React.ReactNode | React.ReactNode[]
  isTor?: boolean
}

function BackgroundView (props: Props) {
  return (
    <Box isTor={props.isTor}>
      <Glow />
      {props.children}
    </Box>
  )
}

export default BackgroundView

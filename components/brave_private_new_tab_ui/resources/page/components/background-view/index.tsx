// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled, { css } from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import textureUrl from '../../assets/bg-texture.svg'

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
  /* Inherited, so all the text on the page avoids orphans and short last lines. */
  text-wrap: pretty;
`

const Glow = styled.div`
  position: absolute;
  z-index: 0;
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

// The Figma texture is black at 50% opacity. Masking with it paints the dots
// in the active palette's shade 30 (primitive-private-window-30 /
// primitive-tor-window-30) without editing the asset. The SVG stretches
// (preserveAspectRatio="none") so the grain covers the whole page.
const Texture = styled.div`
  position: absolute;
  inset: 0;
  z-index: 0;
  pointer-events: none;
  background: var(--window-surface-stronger);
  mask-image: url('${textureUrl}');
  mask-repeat: no-repeat;
  mask-position: center;
  mask-size: 100% 100%;
  mask-mode: alpha;
`

// Positioned layers paint above in-flow content, so the page sits in its own
// stacking context above the texture and the glow.
const Foreground = styled.div`
  position: relative;
  z-index: 1;
  display: flex;
  flex-direction: column;
  flex: 1;
`

interface Props {
  children: React.ReactNode | React.ReactNode[]
  isTor?: boolean
}

function BackgroundView (props: Props) {
  return (
    <Box isTor={props.isTor}>
      <Glow />
      <Texture />
      <Foreground>{props.children}</Foreground>
    </Box>
  )
}

export default BackgroundView

// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Anchor = styled('div')`
  position: relative;
`

export const Tooltip = styled('div').attrs({
  role: 'tooltip'
})`
  --arrow-size: 17px;
  --arrow-width: calc(var(--arrow-size) * 1.41421356237);
  --arrow-width-half: calc(var(--arrow-width) * .5);
  --arrow-start: calc(100% - 50px);
  --arrow-bottom-clip: calc(100% - var(--arrow-width-half));
  position: absolute;
  bottom: calc(100% + 20px);
  right: -20px;
  border-radius: 6px;
  min-width: 250px;
  background: linear-gradient(305.95deg, #BF14A2 0%, #F73A1C 98.59%);
  padding: 24px 26px;
  color: white;

  [dir=rtl] & {
    --arrow-start: 14px;
    left: -5px;
    right: unset;
  }

  /* An arrow that supports any linear-gradient or background-image flowing
    in to the arrow area. We accomplish this using a clip-path. We use a copy
    of the element (via a pseudo element) so that we can keep the rounded corners
    on the original and clip the copy to only the "arrow". */
  &:before {
    content: " ";
    position: absolute;
    top: 0;
    bottom: calc(-1 * var(--arrow-width-half));
    left: 0;
    right: 0;
    background: linear-gradient(305.95deg, #BF14A2 0%, #F73A1C 98.59%);
    clip-path: polygon(
      calc(var(--arrow-start) + var(--arrow-width)) var(--arrow-bottom-clip),
      calc(var(--arrow-start) + var(--arrow-width-half)) 100%,
      var(--arrow-start) var(--arrow-bottom-clip)
    );
  }
`

export const Title = styled('h3')`
  font-family: Poppins;
  font-size: 16px;
  font-weight: 600;
  line-height: 133%;
  letter-spacing: 0.04em;
  margin: 0 0 4px 0;
`

export const TitleIcon = styled('div')`
  display: inline-block;
  vertical-align: sub;
  margin-inline-end: 6px;
  width: 18px;
  height: 18px;
`

export const Body = styled('p')`
  font-family: Poppins;
  font-size: 12px;
  font-weight: 600;
  line-height: 150%;
  letter-spacing: 0.01em;
  margin-bottom: 16px;
  a {
    text-decoration: underline;
    color: inherit;
  }
`

export const CloseButton = styled('button')`
  appearance: none;
  background: none;
  border: none;
  position: absolute;
  padding: 4px;
  box-sizing: content-box;
  margin: 0;
  right: 12px;
  top: 12px;
  width: 12px;
  height: 12px;
  cursor: pointer;
  border-radius: 100%;
  outline: none;
  transition: background .12s ease-in-out, box-shadow .12s ease-in-out;

  [dir=rtl] & {
    right: unset;
    left: 12px;
  }

  :hover, :focus-visible {
    background: rgba(255, 255, 255, .3);
  }

  :active {
    box-shadow: 0 0 0 4px rgba(255, 255, 255, .6);
  }
`

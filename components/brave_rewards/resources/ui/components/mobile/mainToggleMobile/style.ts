/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'

export const StyledWrapper = styled<{}, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.body};
  display: flex;
  width: 100%;
  background-color: #fff;
  justify-content: space-between;
  align-items: center;
  position: fixed;
  top: 0;
  left: 0;
  height: 61px;
  z-index: 2;
  box-shadow: 0 2px 4px  rgba(0,0,0, 0.2);
  padding: 0 16px;
`

export const StyledLeft = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
`

export const StyledRight = styled<{}, 'div'>('div')`
  display: flex;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  color: #4B4C5C;
  font-size: 22px;
  font-weight: 600;
  font-family: ${p => p.theme.fontFamily.heading};

  @media (max-width: 375px) {
    word-wrap: break-word;
  }
`

export const StyledLogoWrapper = styled<{}, 'div'>('div')`
  width: 24px;
  height: 24px;
  margin: 8px;
`

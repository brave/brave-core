// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import OutlineButton from '../../../../outlineButton'

export const BatAdLabel = styled('a')`
  z-index: 2;
  pointer-events: auto;
  position: absolute;
  top: 8px;
  right: 6px;
  background: #FFFFFF;
  border: 1px solid rgba(76, 84, 210, 0.33);
  border-radius: 4px;
  backdrop-filter: blur(10px) brightness(.7);
  padding: 0 2px;
  display: flex;
  align-items: center;
  white-space: nowrap;
  color: ${p => p.theme.color.interactive05};
  font: 500 12px ${p => p.theme.fontFamily.heading};
  /* line-height: 18px; */
  ::before {
    width: 15px;
    height: 15px;
    margin-right: 1px;
    margin-top: -1px;
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32' height='32px' width='32px' x='0px' y='0px'%3E%3Cpath fill-rule='evenodd' xml:space='preserve' fill='%23fff' d='M9.61 23.25h12.78L16 12 9.61 23.25z'%3E%3C/path%3E%3Cpath d='M3 26.8l7.67-4.52L16 13V4a.45.45 0 0 0-.38.28l-6.27 11-6.26 11a.48.48 0 0 0 0 .48' fill='%23ff4724' fill-rule='evenodd'%3E%3C/path%3E%3Cpath d='M16 4v9l5.29 9.31L29 26.8a.48.48 0 0 0-.05-.48l-6.26-11-6.27-11A.45.45 0 0 0 16 4' fill='%239e1f63' fill-rule='evenodd'%3E%3C/path%3E%3Cpath d='M29 26.8l-7.67-4.52H10.71L3 26.8a.47.47 0 0 0 .43.2h25.1a.47.47 0 0 0 .43-.2' fill='%23662d91' fill-rule='evenodd'%3E%3C/path%3E%3C/svg%3E");
    background-size: contain;
    content: '';
    display: block;
    flex-shrink: 1;
  }
`

export const Header = styled('div')`
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 22px;
`

export const CallToAction = styled(OutlineButton)`
  flex: 0 0 auto;
`

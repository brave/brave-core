/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

import defaultBackgroundURL from '../assets/default_background.svg'

export const root = styled.div`
  --creator-background-image-url: url(${defaultBackgroundURL});

  display: flex;
  flex-direction: column;
  gap: 32px;
`

export const avatar = styled.div`
  height: 160px;
  width: 160px;
  border-radius: 50%;
  background: no-repeat center/cover var(--creator-avatar-image-url, none);
`

export const title = styled.div`
  display: flex;
  align-items: center;
  gap: 10px;
`

export const name = styled.div`
  font-weight: 500;
  font-size: 28px;
  line-height: 40px;
  color: ${leo.color.text.primary};
`

export const verifiedCheck = styled.div`
  position: relative;
  height: 20px;
  width: 20px;

  --leo-icon-size: 22px;

  .tooltip {
    visibility: hidden;
    transition: visibility 0s linear 300ms;
  }

  &:hover {
    .tooltip {
      visibility: visible;
    }
  }
`

export const text = styled.div`
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.secondary};
`

export const links = styled.div`
  display: flex;
  align-items: center;
  gap: 24px;
  color: ${leo.color.icon.default};

  a {
    height: 18px;
    width: 18px;
    color: ${leo.color.icon.default};
  }
`

export const linkDivider = styled.div`
  height: 18px;
  width: 1px;
  background: ${leo.color.divider.subtle};

  @media (prefers-color-scheme: dark) {
    background: ${leo.color.divider.strong};
  }
`

export const background = styled.div`
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 264px;
  z-index: -1;
  background:
    no-repeat top center/cover var(--creator-background-image-url),
    ${leo.color.gray['10']};

  &::before {
    content: '';
    position: absolute;
    top: 108px;
    left: 0;
    right: 0;
    height: 560px;
    z-index: -2;
    background:
      linear-gradient(180deg, #FFFFFF 40.99%, rgba(255, 255, 255, 0) 69.84%),
      no-repeat top center/cover var(--creator-background-image-url);
    opacity: 0.1;
    filter: blur(20px);
    transform: matrix(1, 0, 0, -1, 0, 0);
  }
`

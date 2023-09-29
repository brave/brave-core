// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

export const AccountBox = styled.div<{
  orb: string
  size?: 'big' | 'medium' | 'small' | 'tiny'
  marginRight?: number
  round?: boolean
}>`
  --box-big: 48px;
  --box-medium: 32px;
  --box-small: 24px;
  --box-tiny: 20px;
  display: flex;
  align-items: center;
  justify-content: center;
  width: ${(p) =>
    p.size
      ? `var(--box-${p.size})`
      : 'var(--box-medium)'
  };
  min-width: ${(p) =>
    p.size
      ? `var(--box-${p.size})`
      : 'var(--box-medium)'
  };
  height: ${(p) =>
    p.size
      ? `var(--box-${p.size})`
      : 'var(--box-medium)'
  };
  border-radius: ${(p) =>
    p.size === 'big' ? 8 : 4}px;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: ${(p) =>
    p.marginRight !== undefined
      ? p.marginRight
      : 0
  }px;
  border-radius: ${(p) => p.round ? '50%': 'unset'};
`

export const AccountIcon = styled(Icon) <{
  size?: 'big' | 'medium' | 'small' | 'tiny'
}>`
  --icon-big: 24px;
  --icon-medium: 20px;
  --icon-small: 16px;
  --icon-tiny: 14px;
  --leo-icon-size: ${(p) =>
    p.size
      ? `var(--icon-${p.size})`
      : 'var(--icon-medium)'
  };
  color: ${leo.color.white};
`

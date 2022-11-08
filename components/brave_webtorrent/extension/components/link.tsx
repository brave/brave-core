// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'

export const Link = styled('a')<{ inactive?: boolean }>`
    text-decoration: none;
    color: var(--interactive2);

    ${p => p.inactive && css`
        color: var(--text3);
        :hover {
            cursor: default;
        }
    `}
`

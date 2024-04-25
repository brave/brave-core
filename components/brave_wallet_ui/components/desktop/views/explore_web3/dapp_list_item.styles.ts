// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import { Row } from '../../../shared/style'

export const StyledWrapper = styled(Row)`
    border-radius: 8px;

    &:hover {
        background-color: ${leo.color.container.highlight};
    }
`

// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { LoaderIcon } from 'brave-ui/components/icons'
import * as React from 'react'
import styled from 'styled-components'

const Container = styled.div`
    height: 32px;
    width: 32px;
`
export default function Spinner () {
    return <Container>
        <LoaderIcon />
    </Container>
}

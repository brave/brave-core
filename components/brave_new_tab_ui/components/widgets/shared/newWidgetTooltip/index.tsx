// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ThemeProvider } from 'styled-components'
import * as Tooltip from './styles'
// import * as Shared from '../styles'

const tempProps = {
    widgetInfo: {
        title: 'Coinbase',
        icon: '',
        description: 'Trade and sell cryptocurrency on this popular exchange.'
    }
}

export const NewWidgetTooltip = (props: any) => {
    const { title, description } = tempProps.widgetInfo;

    return (
        <ThemeProvider theme={{
            primary: "#FB542B",
            secondary: "#FFFFFF"
        }}>
            <Tooltip.Wrapper>
                <Tooltip.CornerLabel>New Card</Tooltip.CornerLabel>
                <Tooltip.Content>
                    <Tooltip.Heading>{title}</Tooltip.Heading>
                    <Tooltip.Body>{description}</Tooltip.Body>
                    <Tooltip.Button>Add {title}</Tooltip.Button>
                </Tooltip.Content>
            </Tooltip.Wrapper>
        </ThemeProvider>
    )
}
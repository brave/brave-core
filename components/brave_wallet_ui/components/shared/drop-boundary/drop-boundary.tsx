// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { Wrapper } from './styles'

interface Props {
    onDrop?: React.DragEventHandler<HTMLDivElement>
    children: (isDraggedOver: boolean) => React.ReactNode
}

export function DropBoundary ({ children, onDrop }: Props) {
    const [isDraggedOver, setIsDraggedOver] = React.useState(false)

    return (
        <Wrapper
            onDragOver={(event) => {
                event.preventDefault()
                setIsDraggedOver(true)
            }}
            onDragLeave={(event) => {
                event.preventDefault()
                setIsDraggedOver(false)
            }}
            onDrop={(event) => {
                setIsDraggedOver(false)
                onDrop?.(event)
            }}
        >
            {children?.(isDraggedOver)}
        </Wrapper>
    )
}

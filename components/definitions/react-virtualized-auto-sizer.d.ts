// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

declare module '@brave/react-virtualized-auto-sizer' {
  import * as React from 'react'
  export interface Size {
    height: number
    width: number
  }

  export interface AutoSizerProps {
    /** Function responsible for rendering children. */
    children: (size: Size) => React.ReactNode

    /** Optional custom CSS class name to attach to root AutoSizer element.    */
    className?: string

    /** Default height to use for initial render; useful for SSR */
    defaultHeight?: number

    /** Default width to use for initial render; useful for SSR */
    defaultWidth?: number

    /** Disable dynamic :height property */
    disableHeight?: boolean

    /** Disable dynamic :width property */
    disableWidth?: boolean

    /** Nonce of the inlined stylesheet for Content Security Policy */
    nonce?: string

    /** Callback to be invoked on-resize */
    onResize?: (size: Size) => void

    /** Optional inline style */
    style?: React.CSSProperties
  }

  export default class extends React.Component<AutoSizerProps> {}
}

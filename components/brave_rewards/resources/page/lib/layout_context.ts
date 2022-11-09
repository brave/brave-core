/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export type LayoutKind = 'narrow' | 'wide'

export const LayoutContext = React.createContext<LayoutKind>('wide')

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'

import { baseStyles } from './base'
import { layoutStyles } from './layout'
import { transitionStyles } from './transitions'
import { importDataStyles } from './import_data'
import { makeYoursStyles } from './make_yours'
import { completeStyles } from './complete'
import { responsiveStyles } from './responsive'

export const style = scoped.css`
  ${baseStyles}
  ${layoutStyles}
  ${transitionStyles}
  ${importDataStyles}
  ${makeYoursStyles}
  ${completeStyles}
  ${responsiveStyles}
`


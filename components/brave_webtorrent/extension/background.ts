/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { init } from './background/webtorrent'
import './background/store'
import './background/events/tabsEvents'
import './background/events/windowsEvents'

init()

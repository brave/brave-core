/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'

import 'emptykit.css'
import '../../../../../ui/webui/resources/fonts/muli.css'
import '../../../../../ui/webui/resources/fonts/poppins.css'

import { App } from '../../adaptive_captcha/components/app'
import { createHost } from '../../adaptive_captcha/lib/host'

function onReady () {
  ReactDOM.render(
    <App host={createHost()} />,
    document.getElementById('adaptive_captcha_root'))
}

if (document.readyState === 'complete') {
  onReady()
} else {
  document.addEventListener('DOMContentLoaded', onReady)
}

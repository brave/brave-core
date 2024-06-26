// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Import web components here. They will be available on the page
// as <leo-{component}></leo-{component}>.
import '@brave/leo/web-components/button'
import '@brave/leo/web-components/dropdown'
import '@brave/leo/web-components/checkbox'
import '@brave/leo/web-components/label'
import '@brave/leo/web-components/progressRing'
import '@brave/leo/web-components/toggle'
import '@brave/leo/web-components/tooltip'
import '@brave/leo/web-components/alert'
import { setIconBasePath } from '@brave/leo/web-components/icon'
import iconsMeta from '@brave/leo/icons/meta'

setIconBasePath('//resources/brave-icons')

// In Chromium UI Nala variables haven't necessarily been included. We
// make sure the variables are imported so the controls look correct.
const variablesUrl = '//resources/brave/leo/css/variables.css'
const variablesLink = document.querySelector(`link[rel=stylesheet][href$="${variablesUrl}"]`)
if (!variablesLink) {
  const link = document.createElement('link')
  link.setAttribute('rel', 'stylesheet')
  link.setAttribute('href', variablesUrl)
  document.head.appendChild(link)
}

window['leoIcons'] = new Set(Object.keys(iconsMeta.icons))

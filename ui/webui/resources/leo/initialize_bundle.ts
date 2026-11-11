// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Import web components here. They will be available on the page
// as <leo-{component}></leo-{component}>.
import '@brave/nala/web-components/alert'
import '@brave/nala/web-components/alertCenter'
import '@brave/nala/web-components/button'
import '@brave/nala/web-components/buttonMenu'
import '@brave/nala/web-components/checkbox'
import '@brave/nala/web-components/dialog'
import '@brave/nala/web-components/dropdown'
import '@brave/nala/web-components/icon'
import '@brave/nala/web-components/input'
import '@brave/nala/web-components/label'
import '@brave/nala/web-components/link'
import '@brave/nala/web-components/progressRing'
import '@brave/nala/web-components/textarea'
import '@brave/nala/web-components/toggle'
import '@brave/nala/web-components/tooltip'
import '@brave/nala/web-components/radioButton'
import { setIconBasePath } from '@brave/nala/web-components/icon'
import iconsMeta from '@brave/nala/icons/meta'

export {
  showAlert as leoShowAlert
} from '@brave/nala/web-components/alertCenter'

// Note: This file is loaded for some extensions, and on those pages we fall back
// to loading icons from the `chrome://` scheme.
const chromeSchemes = ['chrome:', 'chrome-untrusted:']
const scheme = chromeSchemes.includes(window.location.protocol)
  ? `${window.location.protocol}//`
  : 'chrome://'

setIconBasePath(`${scheme}resources/brave-icons`)

// In Chromium UI Nala variables haven't necessarily been included. We
// make sure the variables are imported so the controls look correct.
const variablesUrl = `resources/brave/css/nala.css`
const variablesLink = document.querySelector(`link[rel=stylesheet][href$="//${variablesUrl}"]`)
if (!variablesLink) {
  const link = document.createElement('link')
  link.setAttribute('rel', 'stylesheet')
  // When we create a link to the variables file, use `scheme` rather than a
  // scheme relative url, to ensure the link loads correctly for extensions.
  link.setAttribute('href', `${scheme}${variablesUrl}`)
  document.head.appendChild(link)
}

declare global {
  interface Window {
    leoIcons: Set<string>
  }
}

window.leoIcons = new Set(Object.keys(iconsMeta.icons))

// Note: We can't use the `if expr` here because this isn't run through the preprocessor.
// iOS doesn't support the color_change_listener, so skip it there.
if (!/iPhone|iPad|iPod/i.test(navigator.userAgent)) {
  import(/* webpackIgnore: true */`${scheme}resources/cr_components/color_change_listener/colors_css_updater.js`)
    .then(({ ColorChangeUpdater }) => {
      ColorChangeUpdater.forDocument().start();
    });
}

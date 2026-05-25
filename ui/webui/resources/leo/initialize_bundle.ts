// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Each `@brave/leo/web-components/<name>` module is re-exported under its
// PascalCase name. The externals function in components/webpack/webpack.config.js
// rewrites imports of `@brave/leo/web-components/<name>` to a real ESM import
// of `(leo.bundle.js).<Name>`, so the module body (and its
// `customElements.define(...)` side effect) runs exactly once — here.
export { default as Alert } from '@brave/leo/web-components/alert'
export { default as AlertCenter } from '@brave/leo/web-components/alertCenter'
export { default as Button } from '@brave/leo/web-components/button'
export { default as ButtonMenu } from '@brave/leo/web-components/buttonMenu'
export { default as Checkbox } from '@brave/leo/web-components/checkbox'
export { default as Collapse } from '@brave/leo/web-components/collapse'
export { default as Dialog } from '@brave/leo/web-components/dialog'
export { default as DialogsContainer } from '@brave/leo/web-components/dialogsContainer'
export { default as Dropdown } from '@brave/leo/web-components/dropdown'
export { default as Floating } from '@brave/leo/web-components/floating'
export { default as FormItem } from '@brave/leo/web-components/formItem'
export { default as Hr } from '@brave/leo/web-components/hr'
export { default as Icon } from '@brave/leo/web-components/icon'
export { default as Input } from '@brave/leo/web-components/input'
export { default as Label } from '@brave/leo/web-components/label'
export { default as Link } from '@brave/leo/web-components/link'
export { default as Menu } from '@brave/leo/web-components/menu'
export { default as Navdots } from '@brave/leo/web-components/navdots'
export { default as Navigation } from '@brave/leo/web-components/navigation'
export { default as NavigationActions } from '@brave/leo/web-components/navigationActions'
export { default as NavigationHeader } from '@brave/leo/web-components/navigationHeader'
export { default as NavigationItem } from '@brave/leo/web-components/navigationItem'
export { default as NavigationMenu } from '@brave/leo/web-components/navigationMenu'
export { default as ProgressBar } from '@brave/leo/web-components/progressBar'
export { default as ProgressRing } from '@brave/leo/web-components/progressRing'
export { default as RadioButton } from '@brave/leo/web-components/radioButton'
export { default as SegmentedControl } from '@brave/leo/web-components/segmentedControl'
export { default as SegmentedControlItem } from '@brave/leo/web-components/segmentedControlItem'
export { default as TabItem } from '@brave/leo/web-components/tabItem'
export { default as Tabs } from '@brave/leo/web-components/tabs'
export { default as Textarea } from '@brave/leo/web-components/textarea'
export { default as Toggle } from '@brave/leo/web-components/toggle'
export { default as Tooltip } from '@brave/leo/web-components/tooltip'

import { setIconBasePath } from '@brave/leo/web-components/icon'
import iconsMeta from '@brave/leo/icons/meta'

export {
  showAlert as leoShowAlert
} from '@brave/leo/web-components/alertCenter'

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
if (!/iPhone|iPad|iPod|Android/i.test(navigator.userAgent)) {
  import(/* webpackIgnore: true */`${scheme}resources/cr_components/color_change_listener/colors_css_updater.js`)
    .then(({ ColorChangeUpdater }) => {
      ColorChangeUpdater.forDocument().start();
    });
}

export * from '@brave/leo/tokens/css/variables'

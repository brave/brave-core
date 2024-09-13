// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Unfortunately, Chromium is midway through migrating from iron-icon to cr-icon
// which means we need to support both. The overrides here and in cr-icon are
// extremely similar, but subtly different (the chromium ==> Nala icon mapping
// is shared). Hopefully in the not too distant future we'll be able to remove
// this, when the settings page is 100% cr-icon.

import { RegisterStyleOverride, RegisterPolymerPrototypeModification } from 'chrome://resources/brave/polymer_overriding.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import 'chrome://resources/brave/leo.bundle.js'

export const leoIcons = (window as any)['leoIcons'] as Set<string>

// Maps Chromium icons to their equivalent Brave icons.
export const iconMap: { [key: string]: string } = {
  'settings:security': 'lock',
  'settings:search': 'search',
  'settings:palette': 'appearance',
  'settings:assignment': 'list-checks',
  'settings:language': 'product-translate',
  'settings:system': 'settings',
  'settings:restore': 'backward',
  'settings:location-on': 'location-on', // location
  'privacy:location-on': 'location-on', // location
  'privacy:location-off': 'location-off', // location off
  'cr:videocam': 'video-camera', // camera
  'privacy:videocam-off': 'video-camera-off', // camera off
  'cr:mic': 'microphone', // microphone
  'privacy:mic': 'microphone', // microphone
  'privacy:mic-off': 'microphone-off', // microphone off
  'privacy:sensors': 'motion-sensor', // motion sensors
  'privacy:sensors-off': 'motion-sensor-off', // motion sensors
  'privacy:notifications': 'notification', // notifications
  'privacy:notifications-off': 'notification-off', // notifications off
  'privacy:code': 'code', // javascript
  'privacy:code-off': 'code-off', // javascript
  'settings:cookie': 'cookie', // cookies
  'privacy:imagesmode': 'image', // images
  'privacy:hide-image': 'image-off', // images off
  'cr:open-in-new': 'launch', // popups & redirects
  'privacy:open-in-new-off': 'launch-off', // popups & redirects off
  'settings:ads': '', // intrusive ads (unused)
  'cr:sync': 'product-sync', // background sync
  'privacy:volume-up': 'volume-on', // sound
  'privacy:volume-off': 'volume-off', // sound off
  'cr:download': 'download', // automatic downloads
  'privacy:protocol-handler': 'protocol-handler', // protocol handler
  'privacy:protocol-handler-off': 'protocol-handler-off', // protocol handler off
  'privacy:file-download-off': 'download-off', // automatic downloads off
  'privacy:piano': 'media-visualizer', // midi devices
  'privacy:piano-off': 'media-visualizer-off', // midi devices off
  'privacy:usb': 'usb', // usb devices
  'privacy:usb-off': 'usb-off', // usb devices off
  'privacy:developer-board': 'cpu-chip', // serial ports
  'privacy:developer-board-off': 'cpu-chip-off', // serial ports off
  'privacy:file-save': 'file-edit', // file editing
  'privacy:file-save-off': 'file-corrupted', // file editing off
  'privacy:videogame-asset': 'sparkles', // hid devices
  'privacy:videogame-asset-off': 'sparkles-off', // hid devices off
  'privacy:sync-saved-locally': 'key-lock', // protected content ids
  'privacy:web-asset-off': 'key-lock-off', // protected content off
  'privacy:content-paste': 'copy', // clipboard
  'privacy:content-paste-off': 'copy-off', // clipboard off
  'privacy:credit-card': 'credit-card', // payment handlers
  'privacy:credit-card-off': 'credit-card-off', // payment handlers
  'settings:bluetooth-scanning': 'bluetooth', // bluetooth scanning
  'settings:bluetooth-off': 'bluetooth-off', // bluetooth off
  'privacy:warning': 'warning-triangle-outline', // insecure content
  'privacy:federated-identity-api': '', // federated identity (unused)
  'privacy:cardboard': 'virtual-reality', // virtual reality & augmented reality
  'privacy:cardboard-off': 'virtual-reality-off', // virtual reality & augmented reality off
  'privacy:select-window': 'windows-open', // window management
  'privacy:select-window-off': 'window-tab-close', // window management off
  'privacy:font-download': 'font-size', // fonts
  'privacy:font-download-off': 'font-size-off', // fonts off
  'privacy:zoom-in': 'search-zoom-in', // zoom levels
  'privacy:drive-pdf': 'file', // pdfs
  'privacy:open-in-browser': 'window', // open in browser
  'settings20:chrome-filled': 'hearts',
  'settings20:incognito-unfilled': 'product-private-window',
  'settings20:lightbulb': 'idea',
  'cr:delete': 'trash', // delete browsing data
  'cr:security': 'lock',
  'privacy:page-info': 'tune', // privacy page additional settings
  'cr:fullscreen': 'fullscreen-on', // automatic fullscreen
  'settings:picture-in-picture': 'picture-in-picture', // picture in picture
  'cr:file-download': 'download',
}

RegisterStyleOverride('iron-icon', html`
  <style>
    :host {
      --leo-icon-size: var(--iron-icon-width, 24px);
      --leo-icon-color: var(--iron-icon-fill-color, currentColor);
    }
  </style>
`)

RegisterPolymerPrototypeModification({
  'iron-icon': (prototype) => {
    const _updateIcon = prototype._updateIcon
    prototype._updateIcon = function (...args: any[]) {
      const removeAllOfType = (type: string) => {
        for (const node of this.shadowRoot.querySelectorAll(type)) node.remove()
      }

      const name = iconMap[this.icon]
      if (name || leoIcons.has(this.icon)) {
        removeAllOfType('svg')
        this._svgIcon = null

        let leoIcon = this.shadowRoot.querySelector('leo-icon')
        if (!leoIcon) {
          leoIcon = document.createElement('leo-icon')
          this.shadowRoot.append(leoIcon)
        }
        leoIcon.setAttribute('name', name ?? this.icon)
      } else {
        removeAllOfType('leo-icon')
        _updateIcon.apply(this, args)
      }
    }
  }
})

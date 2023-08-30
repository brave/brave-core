// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { RegisterStyleOverride, RegisterPolymerPrototypeModification } from 'chrome://resources/brave/polymer_overriding.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import 'chrome://resources/brave/leo.bundle.js'

const leoIcons = (window as any)['leoIcons'] as Set<string>

// Maps Chromium icons to their equivalent Brave icons.
const iconMap: { [key: string]: string } = {
  'cr:security': 'lock',
  'cr:search': 'search',
  'settings:palette': 'appearance',
  'settings:assignment': 'list-checks',
  'settings:language': 'product-translate',
  'settings:build': 'settings',
  'settings:restore': 'backward',
  'settings:location-on': 'location-on', // location
  'settings:location-off': 'location-off', // location off
  'cr:videocam': 'video-camera', // camera
  'settings:videocam-off': 'video-camera-off', // camera off
  'cr:mic': 'microphone', // microphone
  'settings:mic-off': 'microphone-off', // microphone off
  'settings:sensors': 'motion-sensor', // motion sensors
  'settings:sensors-off': 'motion-sensor-off', // motion sensors
  'settings:notifications': 'notification', // notifications
  'settings:notifications-off': 'notification-off', // notifications off
  'settings:code': 'code', // javascript
  'settings:code-off': 'code-off', // javascript
  'settings:cookie': 'cookie', // cookies
  'settings:photo': 'image', // images
  'settings:photo-off': 'image-off', // images off
  'cr:open-in-new': 'launch', // popups & redirects
  'settings:ads': '', // intrusive ads (unused)
  'cr:sync': 'sync', // background sync
  'settings:volume-up': 'volume-on', // sound
  'settings:volume-up-off': 'volume-off', // sound off
  'cr:file-download': 'download', // automatic downloads
  'settings:file-download-off': 'download-off', // automatic downloads off
  'settings:midi': 'media-visualizer', // midi devices
  'settings:midi-off': 'media-visualizer-off', // midi devices off
  'settings:usb': 'usb', // usb devices
  'settings:usb-off': 'usb-off', // usb devices off
  'settings:serial-port': 'cpu-chip', // serial ports
  'settings:serial-port-off': 'cpu-chip-off', // serial ports off
  'settings:save-original': 'file-edit', // file editing
  'settings:file-editing-off': 'file-corrupted', // file editing off
  'settings:hid-device': 'sparkles', // hid devices
  'settings:hid-device-off': 'sparkles-off', // hid devices off
  'settings:protected-content': 'key-lock', // protected content ids
  'settings:protected-content-off': 'key-lock-off', // protected content off
  'settings:clipboard': 'copy', // clipboard
  'settings:clipboard-off': 'copy-off', // clipboard off
  'settings:payment-handler': 'credit-card', // payment handlers
  'settings:payment-handler-off': 'credit-card-off', // payment handlers
  'settings:bluetooth-scanning': 'bluetooth', // bluetooth scanning
  'settings:bluetooth-off': 'bluetooth-off', // bluetooth off
  'settings:insecure-content': 'warning-triangle-outline', // insecure content
  'settings:federated-identity-api': '', // federated identity (unused)
  'settings:vr-headset': 'virtual-reality', // virtual reality & augmented reality
  'settings:vr-headset-off': 'virtual-reality-off', // virtual reality & augmented reality off
  'settings:window-management': 'windows-open', // window management
  'settings:window-management-off': 'window-tab-close', // window management off
  'settings:local-fonts': 'font-size', // fonts
  'settings:local-fonts-off': 'font-size-off', // fonts off
  'settings:zoom-in': 'search-zoom-in', // zoom levels
  'settings:pdf': 'file', // pdfs
  'settings:open-in-browser': 'window', // open in browser

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
        _updateIcon.apply(this, ...args)
      }
    }
  }
})

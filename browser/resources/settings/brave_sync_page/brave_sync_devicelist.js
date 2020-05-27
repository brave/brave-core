// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * @fileoverview
 * 'settings-brave-sync-devicelist' is the control which fetches, displays
 * and updates the list of devices in the sync chain.
 */

cr.exportPath('settings');

Polymer({
  is: 'settings-brave-sync-devicelist',

  behaviors: [
    WebUIListenerBehavior,
  ],

  properties: {
    /**
     * List of devices in sync chain
     * @private
     */
    deviceList_: Array,
  },

  created: function() {
  },

  attached: async function() {
    const deviceList = await cr.sendWithPromise('SyncGetDeviceList')
    this.addWebUIListener('device-info-changed', this.handleDeviceInfo_.bind(this))
    this.handleDeviceInfo_(deviceList)
  },

  handleDeviceInfo_: function(deviceList) {
    console.log('got device list', deviceList)
    this.deviceList_ = deviceList
  },

  getDeviceDisplayDate: function(device) {
    const displayDate = new Date(0)
    displayDate.setUTCSeconds(device.lastUpdatedTimestamp)
    return displayDate.toDateString()
  }
});

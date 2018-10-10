/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
'use strict';

Polymer({
  is: 'settings-brave-personalization-options',

  properties: {
    webRTCPolicies_: {
      readOnly: true,
      type: Array,
      value: function() {
        return [
          {value: 'default', name: loadTimeData.getString('webRTCDefault')},
          {value: 'default_public_and_private_interfaces', name: loadTimeData.getString('defaultPublicAndPrivateInterfaces')},
          {value: 'default_public_interface_only', name: loadTimeData.getString('defaultPublicInterfaceOnly')},
          {value: 'disable_non_proxied_udp', name: loadTimeData.getString('disableNonProxiedUdp')}
        ]
      },
    },

    webRTCPolicy_: String,
  },

  /** @private {?settings.BravePrivacyBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BravePrivacyBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.onWebRTCPolicyChange_ = this.onWebRTCPolicyChange_.bind(this)
    this.browserProxy_.getWebRTCPolicy().then(policy => {
      this.webRTCPolicy_ = policy;
    });
  },

  /**
   * @param {string} policy1
   * @param {string} policy2
   * @return {boolean}
   * @private
   */
  webRTCPolicyEqual_: function(policy1, policy2) {
    return policy1 === policy2;
  },

  onWebRTCPolicyChange_: function() {
    this.browserProxy_.setWebRTCPolicy(this.$.webRTCPolicy.value);
  },
});
})();

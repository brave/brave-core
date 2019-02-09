// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * @fileoverview 'settings-clear-browsing-data-dialog' allows the user to
 * delete Rewards data that has been stored by Brave Rewards.
 */
Polymer({
  is: 'settings-brave-clear-rewards-data-dialog',

  behaviors: [WebUIListenerBehavior],

  properties: {
    /**
     * Preferences state.
     */
    prefs: {
      type: Object,
      notify: true,
    },

    /**
     * Results of rewards data counters, keyed by the suffix of
     * the corresponding data type deletion preference, as reported
     * by the C++ side.
     * @private {!Object<string>}
     */
    counters_: {
      type: Object,
      // Will be filled as results are reported.
      value: function() {
        return {};
      }
    },

    /** @private */
    autoContributeDisabled_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    clearingInProgress_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    clearButtonDisabled_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    rewardsClearable_: {
      type: Boolean,
      value: true,
    },

    /** @private */
    contributionInProgress_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    clearAllRewardsModalActive_: {
      type: Boolean,
      value: false,
    },

    shouldShowFooter_: {
      type: Boolean,
      value: false,
    },

    cancelButtonDisabled_: {
      type: Boolean,
      value: false,
    },

    showSpinner_: {
      type: Boolean,
      value: false,
    },
  },

  listeners: {
    'settings-boolean-control-change': 'updateClearButtonState_'
  },

  /** @private {settings.ClearRewardsDataBrowserProxyImpl} */
  rewardsProxy_: null,

  /** @override */
  ready: function() {
    this.addWebUIListener(
        'update-counter-text', this.updateCounterText_.bind(this));
    this.addWebUIListener(
        'update-contribution-in-progress',
          this.updateContributionInProgress_.bind(this));

  },

  /** @override */
  attached: function() {
    const data = this.$.rewardsData;
    this.rewardsProxy_ =
        settings.ClearRewardsDataBrowserProxyImpl.getInstance();
    this.rewardsProxy_.initialize().then(() => {
      this.$.clearRewardsDataDialog.showModal();
      this.rewardsClearable_ = !this.contributionInProgress_;
      this.configureCheckBoxes_(data);
      this.clearButtonDisabled_ = this.getSelectedDataTypes_(data).length == 0;
      this.rewardsProxy_.isContributionInProgress().then((in_progress) => {
        this.updateContributionInProgress_(in_progress);
      });
    });
  },

  /**
   * Returns true if either clearing is in progress or no data type is selected.
   * @param {boolean} clearingInProgress
   * @param {boolean} clearButtonDisabled
   * @return {boolean}
   * @private
   */
  isClearButtonDisabled_: function(clearingInProgress, clearButtonDisabled) {
    return clearingInProgress || clearButtonDisabled;
  },

  /**
   * Disables the Clear Data button if no data type is selected.
   * @private
   */
  updateClearButtonState_: function() {
    const data = this.$.rewardsData;
    if (!data) {
      return;
    }
    this.configureCheckBoxes_(data);
    this.clearButtonDisabled_ = this.getSelectedDataTypes_(data).length == 0;
  },

  isAutoContributeDisabled_: function() {
    return this.autoContributeDisabled_ ||
           !this.rewardsClearable_ ||
           this.clearingInProgress_;
  },

  /**
   * Updates the text of a rewards data counter corresponding to the given
   * preference.
   * @param {string} prefName Rewards data type deletion preference.
   * @param {string} text The text with which to update the counter
   * @private
   */
  updateCounterText_: function(prefName, text) {
    // Data type deletion preferences are named "brave.clear_data.<datatype>".
    // Strip the common prefix, i.e. use only "<datatype>".
    const matches = prefName.match(/^brave\.clear_data\.(\w+)$/);
    this.set('counters_.' + assert(matches[1]), text);
  },

  /**
   * Returns a list of selected data types.
   * @param {!HTMLElement} data
   * @return {!Array<string>}
   * @private
   */
  getSelectedDataTypes_: function(data) {
    const checkboxes = data.querySelectorAll('settings-checkbox');
    const dataTypes = [];
    checkboxes.forEach((checkbox) => {
      if (checkbox.checked && !checkbox.hidden) {
        dataTypes.push(checkbox.pref.key);
      }
    });
    return dataTypes;
  },

  /**
   * Clears rewards data.
   * @private
   */
  clearRewardsData_: function() {
    let rewardsModalActive = false;
    this.clearingInProgress_ = this.showSpinner_ = true;
    const data = this.$.rewardsData;
    const checkboxes = data.querySelectorAll('settings-checkbox');
    if (!this.clearAllRewardsModalActive_) {
      checkboxes.forEach((checkbox) => {
        if (checkbox.id == 'rewardsAllData' && checkbox.checked) {
          // pause here and show warning for clearing all rewards data
          this.$.confirmClearRewardsDataDialog.showModal();
          rewardsModalActive = true;
          this.clearAllRewardsModalActive_ = true;
          this.clearingInProgress_ = this.showSpinner_ = false;
        }
      });
    } else {
      // user confirmed to go ahead and clear
      rewardsModalActive = false;
    }

    if (!rewardsModalActive) {
      const dataTypes = this.getSelectedDataTypes_(data);

      this.rewardsProxy_.clearRewardsData(dataTypes)
          .then((success) => {
            this.clearingInProgress_ = this.showSpinner_ = false;
            this.$.clearRewardsDataDialog.close();
          });
    }
  },

  /** @private */
  onCancelTap_: function() {
    this.$.clearRewardsDataDialog.cancel();
  },

  confirmDialogCancel_: function() {
    this.$.confirmClearRewardsDataDialog.cancel();
  },

  updateContributionInProgress_: function(in_progress) {
    this.shouldShowFooter_ =
    this.contributionInProgress_ =
    this.clearingInProgress_ = in_progress;
    this.showSpinner_ = false;
    this.configureCheckBoxes_(this.$.rewardsData);
  },

  /** @private */
  configureCheckBoxes_: function(data) {
    this.cancelButtonDisabled_ = this.clearingInProgress_ && !this.contributionInProgress_;
    const checkboxes = data.querySelectorAll('settings-checkbox');
    checkboxes.forEach((checkbox) => {
      if (checkbox.id == 'rewardsAutoContribute' ||
          checkbox.id == 'rewardsAllData') {
        if (!this.rewardsClearable_) {
          checkbox.checked = false;
        }
        if (checkbox.id == 'rewardsAutoContribute') {
          checkboxes.forEach((allCb) => {
            if (allCb.id == 'rewardsAllData') {
              if (allCb.checked) {
                this.autoContributeDisabled_ = true;
                checkbox.checked = false;
              } else {
                this.autoContributeDisabled_ = !this.rewardsClearable_ || this.contributionInProgress_;
              }
            }
          });
        }
      }
    });
  },
});

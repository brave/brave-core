// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.js';

import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {CrLitElement} from 'chrome://resources/lit/v3_0/lit.rollup.js';

import type {BraveTabSearchApiProxy} from '../tab_search_api_proxy.js';
import {TabSearchApiProxyImpl} from '../tab_search_api_proxy.js';
import {TabSearchSection} from '../tab_search.mojom-webui.js';
import type {TabOrganizationSession} from '../tab_search.mojom-webui.js';

import {getHtml} from './auto_tab_groups_page.html.js';
import {getCss} from './auto_tab_groups_page.css.js';

export class AutoTabGroupsPageElement extends CrLitElement {
  static get is() {
    return 'auto-tab-groups-page'
  }

  private apiProxy_: BraveTabSearchApiProxy = TabSearchApiProxyImpl.getInstance() as BraveTabSearchApiProxy;
  protected topics_: string[] = [];
  protected topic: string = '';
  protected undoTopic_: string = '';

  private visibilityChangedListener_: () => void;
  private elementVisibilityChangedListener_: IntersectionObserver;
  private wasInactive_: boolean = false;

  static override get properties() {
    return {
      availableHeight: {type: Number},
      showBackButton: {type: Boolean},
      topic: {type: String},
      topics_: {type: Array},
      undoTopic_: {type: String},
      isLoadingTopics: {type: Boolean},
    };
  }

  availableHeight: number = 0;
  showBackButton: boolean = false;
  isLoadingTopics: boolean = false

  static override get styles() {
    return getCss();
  }

  constructor() {
    super();

    this.visibilityChangedListener_ = () => {
      if (document.visibilityState === 'visible' && !this.wasInactive_) {
        console.error('update due to doc visibility changed to visible and not inactive');
        // this.updateSuggestedTopics_();
      }
    }

    this.elementVisibilityChangedListener_ =
        new IntersectionObserver((entries, _observer) => {
          entries.forEach(entry => {
            this.onElementVisibilityChanged_(entry.intersectionRatio > 0);
          });
        }, {root: document.documentElement});

  }

  protected onTopicInputChanged_(e: any) {
    this.topic = e.value;
  }

  protected onFocusTabsClicked_() {
    this.getFocusTabs_(this.topic);
  }

  protected onUndoClicked_() {
    this.apiProxy_.undoFocusTabs().then(() => {
      this.undoTopic_ = '';
    });
  }

  private getFocusTabs_(topic: string) {
    this.undoTopic_ = '';
    this.apiProxy_.getFocusTabs(topic).then(({windowCreated, error}) => {
      if (error) {
        console.error('Error focusing tabs: ' + error.message);
        // TODO: show error message to user
        return;
      }
      if (windowCreated) {
        this.undoTopic_ = topic;
      }
    });
  }

  private updateSuggestedTopics_() {
    this.isLoadingTopics = true
    this.apiProxy_.getSuggestedTopics().then(({topics, error}) => {
      if (error) {
        console.error('Error getting suggested topics: ' + error.message);
        // TODO: show error message to user
        return;
      }
      this.onSuggestTopicsUpdated(topics);
      this.isLoadingTopics = false
    });
  }

  private onSuggestTopicsUpdated(topics: string[]) {
    this.topics_ = topics;
  }

  protected getTopicId_(index: number): string {
    return 'topicId' + index;
  }

  protected getTopicWithoutEmoji_(topic: string): string {
    return topic.replace(/[^\x20-\x7F]/g, '').trimStart();
  }

  protected getTopicEmoji_(topic: string): string {
    return topic.match(/^\p{Emoji}/u)![0]
  }

  protected onTopicClicked_(index: number) {
    if (this.topics_[index]) {
      this.getFocusTabs_(this.topics_[index]);
    }
  }

  override render() {
    return getHtml.bind(this)();
  }

  override connectedCallback() {
    super.connectedCallback();
    document.addEventListener('visibilitychange', this.visibilityChangedListener_);
    this.elementVisibilityChangedListener_.observe(this);

    this.apiProxy_.getTabSearchSection().then(
        ({section}) => this.wasInactive_ =
            section !== TabSearchSection.kOrganize);

    // if (document.visibilityState === 'visible' && !this.wasInactive_) {
    //   console.error('update when connected due to doc visibility is visible and not inactive');
    this.updateSuggestedTopics_();
    // }
  }

  override disconnectedCallback() {
    super.disconnectedCallback();
    document.removeEventListener('visibilitychange', this.visibilityChangedListener_);
    this.elementVisibilityChangedListener_.disconnect();
  }

  protected getTitle_(): string {
    return loadTimeData.getString('tabOrganizationTitle');
  }

  protected getSubtitle_(): string {
    return loadTimeData.getString('tabOrganizationSubtitle');
  }

  protected getSuggestedTopicsSubtitle_(): string {
    return loadTimeData.getString('tabOrganizationSuggestedTopicsSubtitle');
  }

  protected getBackButtonAriaLabel_(): string {
    return loadTimeData.getStringF('backButtonAriaLabel', this.getTitle_());
  }

  protected getSubmitButtonLabel_(): string {
    return loadTimeData.getString('tabOrganizationSubmitButtonLabel');
  }

  protected getUndoButtonLabel_(): string {
    return loadTimeData.getString('tabOrganizationUndoButtonLabel');
  }

  protected getWindowCreatedMessage_(): string {
    return loadTimeData.getStringF('tabOrganizationWindowCreatedMessage', this.undoTopic_);
  }

  protected getTopicInputPlaceholder_(): string {
    return loadTimeData.getString('tabOrganizationTopicInputPlaceholder');
  }

  protected getFooterMessage_(): string {
    return loadTimeData.getStringF('tabOrganizationFeatureExplanation');
  }

  protected getLearnMoreLabel_(): string {
    return loadTimeData.getString('tabOrganizationLearnMoreLabel');
  }

  protected onLearnMoreClicked_() {
    this.apiProxy_.openHelpPage();
  }

  protected onGoPremiumClicked_() {
    this.apiProxy_.openLeoGoPremiumPage();
  }

  override focus() {
    if (this.showBackButton) {
      const backButton = this.shadowRoot!.querySelector('cr-icon-button')!;
      backButton.focus();
    } else {
      super.focus();
    }
  }

  protected onBackClick_() {
    this.fire('back-click');
  }

  private onElementVisibilityChanged_(visible: boolean) {
    if (visible && this.wasInactive_) {
      console.error('update due to element visibility changed to visible and was inactive');
      // this.updateSuggestedTopics_();
    } else if (!visible) {
      console.error('element visibility changed to invisible');
      this.wasInactive_ = true;
    }
  }

  // Shim for Chromium auto_tab_groups_page test.
  setSessionForTesting(_session: TabOrganizationSession) {
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'auto-tab-groups-page': AutoTabGroupsPageElement;
  }
}

customElements.define(AutoTabGroupsPageElement.is, AutoTabGroupsPageElement);

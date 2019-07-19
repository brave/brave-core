/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * @unrestricted
 */
Elements.PageGraphInfoWidget = class extends UI.ThrottledWidget {
  constructor() {
    super(true /* isWebComponent */);
    this.registerRequiredCSS('../../../../../brave/third_party/blink/renderer/devtools/front_end/elements/pageGraphInfoWidget.css');

    UI.context.addFlavorChangeListener(SDK.DOMNode, this._setNode, this);
    this._node = UI.context.flavor(SDK.DOMNode);
    this.update();
  }

  /**
   * @param {!Common.Event} event
   */
  _setNode(event) {
    this._node = /** @type {?SDK.DOMNode} */ (event.data);
    this.update();
  }

  /**
   * @override
   * @protected
   * @return {!Promise<undefined>}
   */
  async doUpdate() {
    if (!this._node) {
      this.contentElement.removeChildren();
      return;
    }

    const pageGraphReport = await this._node.reportPageGraphInfo();
    if (!pageGraphReport) {
      return;
    }

    this.contentElement.removeChildren();

    const reportElement = this.contentElement.createChild('div', 'page-graph-report');
    for (let reportItem of pageGraphReport) {
      const reportItemElement = createElementWithClass('div', 'page-graph-report-item');
      reportItemElement.textContent = reportItem;
      reportElement.appendChild(reportItemElement);
    }
  }
};

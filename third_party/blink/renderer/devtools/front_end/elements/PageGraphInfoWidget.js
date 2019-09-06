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

    const toolbar = new UI.Toolbar('page-graph-toolbar', this.contentElement);

    this._refreshButton =
        new UI.ToolbarButton(null, 'largeicon-refresh', Common.UIString('Refresh'));
    this._refreshButton.addEventListener(UI.ToolbarButton.Events.Click, this.update.bind(this));
    toolbar.appendToolbarItem(this._refreshButton);

    this._exportButton =
        new UI.ToolbarButton(null, 'largeicon-download', Common.UIString('Save Full Page Graph'));
    this._exportButton.addEventListener(UI.ToolbarButton.Events.Click, this._export.bind(this));
    toolbar.appendToolbarItem(this._exportButton);

    this._reportElement = this.contentElement.createChild('div', 'page-graph-report');

    UI.context.addFlavorChangeListener(SDK.DOMNode, this._setNode, this);
    this._node = UI.context.flavor(SDK.DOMNode);
    this.update();
  }

  /**
   * @override
   * @return {!Array<!UI.ToolbarItem>}
   */
  toolbarItems() {
    return this._toolbarItems;
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
    try {
      this._refreshButton.setEnabled(false);

      if (!this._node) {
        this._reportElement.removeChildren();
        return;
      }

      const pageGraphReport = await this._node.reportPageGraphInfo();
      if (!pageGraphReport) {
        return;
      }

      this._reportElement.removeChildren();

      for (let reportItem of pageGraphReport) {
        const reportItemElement = createElementWithClass('div', 'page-graph-report-item');
        reportItemElement.textContent = reportItem;
        this._reportElement.appendChild(reportItemElement);
      }
    } finally {
      this._refreshButton.setEnabled(true);
    }
  }

  /**
   * @override
   * @private
   * @return {!Promise<undefined>}
   */
  async _export() {
    try {
      this._exportButton.setEnabled(false);

      const target = this._node.domModel().target();
      const pageAgent = target.pageAgent();
      const runtimeAgent = target.runtimeAgent();

      const getResourceTreeResponse = await pageAgent.invoke_getResourceTree();
      if (getResourceTreeResponse[Protocol.Error]) {
        UI.MessageDialog.show(Common.UIString('Failed to retrieve page frame tree: %s',
                                              getResourceTreeResponse[Protocol.Error]));
        return;
      }
      const frameId = getResourceTreeResponse.frameTree.frame.id;

      const generatePageGraphResponse = await pageAgent.invoke_generatePageGraph();
      if (generatePageGraphResponse[Protocol.Error]) {
        UI.MessageDialog.show(Common.UIString('Failed to generate page graph XML: %s',
                                              generatePageGraphResponse[Protocol.Error]));
        return;
      }
      const pageGraph = generatePageGraphResponse.data;

      const createIsolatedWorldResponse = await pageAgent.invoke_createIsolatedWorld({ frameId });
      if (createIsolatedWorldResponse[Protocol.Error]) {
        UI.MessageDialog.show(Common.UIString('Failed to create script execution context: %s',
                                              createIsolatedWorldresponse[Protocol.Error]));
        return;
      }
      const contextId = createIsolatedWorldResponse.executionContextId;

      const expression = `
          const pageGraph = ${JSON.stringify(pageGraph)};
          const pageGraphURL =
              URL.createObjectURL(new Blob([pageGraph], { type: 'application/xml' }));

          const exportJS = \`(function() {
              document.title = 'Page Graph Export';

              const linkElement = document.createElement('a');
              linkElement.href = \${JSON.stringify(pageGraphURL)};
              linkElement.download = \${JSON.stringify(document.title)} + '.graphml';
              linkElement.click();
          })()\`;

          open('javascript:' + encodeURIComponent(exportJS));
      `;

      const evaluateResponse = await runtimeAgent.invoke_evaluate({
          expression,
          contextId,
          userGesture: true
      });
      if (evaluateResponse[Protocol.Error]) {
        UI.MessageDialog.show(Common.UIString('Failed to evaluate export script in page: %s',
                                              evaluateResponse[Protocol.Error]));
      }
    } finally {
      this._exportButton.setEnabled(true);
    }
  }
};

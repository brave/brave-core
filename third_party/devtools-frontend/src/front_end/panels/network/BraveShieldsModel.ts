/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Common from '../../core/common/common.js'
import * as Root from '../../core/root/root.js'
import { DOMNode } from '../../core/sdk/DOMModel.js'
import * as SDK from '../../core/sdk/sdk.js'
import type * as ProtocolProxyApi from '../../generated/protocol-proxy-api.js'
import * as Protocol from '../../generated/protocol.js'
import { BraveModel } from './BraveModel.js'

export interface Filter {
  selector: string
  nodes: DOMNode[] | null
}

export class BraveShieldsModel extends BraveModel {
  readonly #domSnapshotAgent: ProtocolProxyApi.DOMSnapshotApi
  readonly #overlayAgent: ProtocolProxyApi.OverlayApi

  constructor(target: SDK.Target.Target) {
    super(target, new Set(['Brave.Shields.CosmeticFilters']))

    this.#domSnapshotAgent = target.domsnapshotAgent()
    this.#overlayAgent = target.overlayAgent()
  }

  highlightNode(node: Protocol.DOM.BackendNodeId): void {
    const highlightConfig = {
      contentColor: Common.Color.PageHighlight.Content.toProtocolRGBA(),
      showInfo: true,
      contrastAlgorithm: Root.Runtime.experiments.isEnabled('apca')
        ? Protocol.Overlay.ContrastAlgorithm.Apca
        : Protocol.Overlay.ContrastAlgorithm.Aa
    }

    void this.#overlayAgent.invoke_hideHighlight()
    void this.#overlayAgent.invoke_highlightNode({
      backendNodeId: node,
      highlightConfig
    })
  }

  async getActiveFilters(filters: string[]): Promise<Filter[]> {
    const domModel = this.target().model(SDK.DOMModel.DOMModel)
    if (!domModel) {
      return []
    }
    const documentNode = await domModel.requestDocument()
    if (!documentNode) {
      return []
    }

    const result: Filter[] = []
    for (const selector of filters) {
      const nodes = await domModel.querySelectorAll(documentNode.id, selector)
      const domNodes = nodes
        ?.map((nodeId) => {
          return domModel.nodeForId(nodeId)
        })
        .filter((node) => node) as DOMNode[] | null
      result.push({ selector: selector, nodes: domNodes })
    }
    return result.sort((a: Filter, b: Filter): number => {
      const ca = a.nodes ? a.nodes.length : 0
      const cb = b.nodes ? b.nodes.length : 0
      return ca - cb
    })
  }
}

SDK.SDKModel.SDKModel.register(BraveShieldsModel, {
  capabilities: SDK.Target.Capability.DOM,
  autostart: false
})

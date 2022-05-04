import {PageCallbackRouter, PageHandlerFactory, PageHandlerRemote} from './federated_internals.mojom-webui.js';

export class FederatedInternalsBrowserProxy {
  handler;
  callbackRouter;

  constructor() {
    this.callbackRouter = new PageCallbackRouter();
    this.handler = new PageHandlerRemote();
    const factory = PageHandlerFactory.getRemote();
    factory.createPageHandler(
        this.callbackRouter.$.bindNewPipeAndPassRemote(),
        this.handler.$.bindNewPipeAndPassReceiver());
  }

  getAdStoreInfo() {
    return this.handler.getAdStoreInfo();
  }

  static getInstance() {
    return instance || (instance = new FederatedInternalsBrowserProxy());
  }

  getCallbackRouter() {
    return this.callbackRouter;
  }
}

let instance = null;
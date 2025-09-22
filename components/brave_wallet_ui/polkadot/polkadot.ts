import 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { getAPIProxy } from '../common/async/bridge'
// import getWalletPageApiProxy from '../page/wallet_page_api_proxy'

// @ts-expect-error
import { get_signature } from 'chrome-untrusted://resources/brave/polkadot_bridge_wasm.bundle.js'

// let walletPageApiProxyInstance: WalletPageApiProxy

// function getWalletPageApiProxy() {
//   if (!walletPageApiProxyInstance) {
//     walletPageApiProxyInstance = new WalletPageApiProxy()
//   }
//   return walletPageApiProxyInstance
// }

const apiProxy = getAPIProxy();

window.addEventListener('message', (msgEvent) => {
  try {
    if (!msgEvent.source) { return; }

    msgEvent.source.postMessage(get_signature(), { targetOrigin: msgEvent.origin });

    (async () => {
      const polkadotWalletService = apiProxy.polkadotWalletService;
      const network = await polkadotWalletService.getNetworkName();
      console.log('retrieved the network!');
      console.log(network);
    })();

  } catch (err) {
    console.error(err);
  }
});

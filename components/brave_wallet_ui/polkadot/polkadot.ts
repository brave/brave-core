import 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import {
  BraveWallet,
} from '../constants/types'
import { getAPIProxy } from '../common/async/bridge'
import { AccountId } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m'

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
      let account: AccountId = {
        coin: BraveWallet.CoinType.DOT,
        keyringId: BraveWallet.KeyringId.kPolkadotTestnet,
        kind: 0,
        address: '',
        accountIndex: 0,
        uniqueKey: `${BraveWallet.CoinType.DOT}_${BraveWallet.KeyringId.kPolkadotTestnet}_0_0`
      };

      const polkadotWalletService = apiProxy.polkadotWalletService;
      const network = await polkadotWalletService.getNetworkName(account);
      console.log('retrieved the network!');
      console.log(network);
    })();

  } catch (err) {
    console.error(err);
  }
});

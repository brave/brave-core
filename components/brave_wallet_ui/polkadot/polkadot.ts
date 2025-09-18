import { PolkadotWalletServiceObserverReceiver } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import {
  BraveWallet,
} from '../constants/types'
import { getAPIProxy } from '../common/async/bridge'
import { AccountId } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m'

// @ts-expect-error
import { get_signature, get_pubkey } from 'chrome-untrusted://resources/brave/polkadot_bridge_wasm.bundle.js'

const apiProxy = getAPIProxy();

class PolkadotWalletServiceObserverImpl {
  receiver: PolkadotWalletServiceObserverReceiver;
  pubkey: string | null;

  constructor() {
    this.receiver = new PolkadotWalletServiceObserverReceiver(this);
    this.pubkey = null;
  }

  getPublicKey(): Promise<{ pubkey: string }> {
    if (!this.pubkey) {
      console.log('pubkey not in cache, generating now via wasm...');
      this.pubkey = get_pubkey();
    } else {
      console.log('pubkey was already in the cache!');
    }
    if (!this.pubkey) { throw 'unreachable'; }
    return Promise.resolve({ pubkey: this.pubkey });
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote();
  }
}

const polkadotImpl = new PolkadotWalletServiceObserverImpl();
apiProxy.polkadotWalletService.addObserver(polkadotImpl.getPendingRemote());

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

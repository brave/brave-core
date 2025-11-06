import { KeyringServiceObserverReceiver, PolkadotWalletServiceObserverReceiver } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import {
  BraveWallet,
} from '../constants/types'
import { getAPIProxy } from '../common/async/bridge'
import { AccountId } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m'

// @ts-expect-error
import { get_signature, get_pubkey, make_keypair, derive_account_keypair } from 'chrome-untrusted://resources/brave/polkadot_bridge_wasm.bundle.js'

const apiProxy = getAPIProxy();

class PolkadotWalletServiceObserverImpl {
  receiver: PolkadotWalletServiceObserverReceiver;
  pubkey: string | null;
  rootkey: number[] | null;

  constructor() {
    this.receiver = new PolkadotWalletServiceObserverReceiver(this);
    this.pubkey = null;
    this.rootkey = null;
  }

  getPublicKey(account: AccountId): Promise<{ pubkey: string }> {
    // if (!this.pubkey) {
    //   console.log('pubkey not in cache, generating now via wasm...');
    //   this.pubkey = get_pubkey();
    // } else {
    //   console.log('pubkey was already in the cache!');
    // }
    // if (!this.pubkey) { throw 'unreachable'; }

    if (!this.rootkey) { throw 1234; }

    let keypair = derive_account_keypair(this.rootkey, account.accountIndex);
    const pubkey = Buffer.from(keypair.slice(64, 64 + 32)).toString('hex');
    return Promise.resolve({ pubkey });
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote();
  }
}

class KeyringServiceObserverImpl {
  receiver: KeyringServiceObserverReceiver;
  polkadotImpl: PolkadotWalletServiceObserverImpl;

  constructor(polkadotImpl: PolkadotWalletServiceObserverImpl) {
    this.receiver = new KeyringServiceObserverReceiver(this);
    this.polkadotImpl = polkadotImpl;
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote();
  }


  walletCreated() {
  }

  walletRestored() {
  }

  walletReset() {
  }

  locked() {
  }

  unlocked() {
    console.log('wallet was unlocked so yay, I guess');
  }

  backedUp() {
  }

  accountsChanged() {
  }

  accountsAdded() {
  }

  autoLockMinutesChanged() {
  }

  selectedWalletAccountChanged(
    account: BraveWallet.AccountInfo,
  ) {
  }

  selectedDappAccountChanged(
    coin: BraveWallet.CoinType,
    account: BraveWallet.AccountInfo | null,
  ) {
  }

  // d333b26eba47cccc1195aece9d8e885d84723338595ea3e53c30374c1d2818099cf6712690629b52ad69765157de57800d7eca5c43b019e5a4584927b01aa2908484900af339e2cf77aa775ee652f72cd4deda1caf7f3ee538ef14f9a0021d3d

  generateWasmKeyrings(seed: number[]) {
    console.log('holy crap, got the seed');
    console.log(seed);
    this.polkadotImpl.rootkey = make_keypair(seed, true);
    console.log(this.polkadotImpl.rootkey);
    console.log(Buffer.from(this.polkadotImpl.rootkey || []).toString('hex'));
  }
}

const polkadotImpl = new PolkadotWalletServiceObserverImpl();
apiProxy.polkadotWalletService.addObserver(polkadotImpl.getPendingRemote());

const keyringServiceImpl = new KeyringServiceObserverImpl(polkadotImpl);
apiProxy.keyringService.addObserver(keyringServiceImpl.getPendingRemote());
console.log(keyringServiceImpl);

window.addEventListener('message', (msgEvent) => {
  try {
    if (!msgEvent.source) { return; }

    // msgEvent.source.postMessage(get_signature(), { targetOrigin: msgEvent.origin });

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
      // const network = await polkadotWalletService.getNetworkName(account);
      // console.log('retrieved the network!');
      // console.log(network);


      const accountInfo = await polkadotWalletService.getAccountBalance(account, "polkadot_testnet");
      console.log(accountInfo);
      const str = JSON.stringify(accountInfo, (_, v) => typeof v === 'bigint' ? v.toString() : v);
      msgEvent.source?.postMessage(str, { targetOrigin: msgEvent.origin });
    })();

  } catch (err) {
    console.error(err);
  }
});

// @ts-expect-error
import { get_signature } from 'chrome-untrusted://resources/brave/polkadot_bridge_wasm.bundle.js'

window.addEventListener('message', (msgEvent) => {
  try {
    if (!msgEvent.source) { return; }

    msgEvent.source.postMessage(get_signature(), { targetOrigin: msgEvent.origin });
    window.parent.postMessage(get_signature());
  } catch (err) {
    console.error(err);
  }
});

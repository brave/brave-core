// @ts-expect-error
import { rawr, get_pubkey } from 'chrome-untrusted://resources/brave/brave_wallet_ui_wasm.bundle.js'

console.log("i probably definitely shouldn't see this at all...");

console.log(get_pubkey());
window.addEventListener('message', (msgEvent) => {
  try {
    console.log('heard the message loud and clear!');
    console.log(msgEvent.data);
    console.log(msgEvent);

    if (!msgEvent.source) { console.log('unable to get the source'); return; }

    msgEvent.source.postMessage(get_pubkey(), { targetOrigin: msgEvent.origin });
    window.parent.postMessage(get_pubkey());
  } catch (err) {
    console.log("error");
    console.log(err);
  }
});

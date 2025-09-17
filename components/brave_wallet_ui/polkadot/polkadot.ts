// @ts-expect-error
import { rawr, get_pubkey } from 'chrome-untrusted://resources/brave/brave_wallet_ui_wasm.bundle.js'

console.log("i probably definitely shouldn't see this at all...");

console.log(get_pubkey());

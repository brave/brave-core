import load, { initialize as init } from "./skus_sdk.js";

let wasmImportURL = undefined

export const initialize = async function() {
  await load(wasmImportURL)
  return await init.apply(this, arguments)
}

export const setWASMImportURL = function(url) {
  wasmImportURL = url
}

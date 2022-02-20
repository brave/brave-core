export function randomFloat() {
    const int = window.crypto.getRandomValues(new Uint32Array(1))[0]
    return int / 2**32
}
export function randomInt () {
    return window.crypto.getRandomValues(new Uint32Array(1))[0]
}

export function randomFloat () {
    return randomInt() / 2 ** 32
}

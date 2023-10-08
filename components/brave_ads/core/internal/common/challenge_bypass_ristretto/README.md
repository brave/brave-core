# Challenge Bypass Ristretto

A facade to reduce the complexity of using [Challenge Bypass Ristretto FFI](https://github.com/brave-intl/challenge-bypass-ristretto-ffi) implementation of the [privacy pass cryptographic protocol](https://www.petsymposium.org/2018/files/papers/issue3/popets-2018-0026.pdf). See [test](challenge_bypass_ristretto_test.cc) for an example of proving and verifying an unblinded token from server to client.

The client prepares random tokens, blinds them so the issuer cannot determine the original token value, and sends them to the issuer. The issuer signs the tokens using a secret key and returns them to the client. The client then reverses the original blind to yield a signed token.

Please add to it!

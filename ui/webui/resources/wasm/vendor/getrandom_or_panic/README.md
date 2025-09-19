# getrandom_or_panic

Addresses one minor conundrum in cryptography crates:

We want end user signers and provers to dependend directly upon
getrandom for system randomness, so that users cannot supply
insecure random number generators.

We need verifiers to run in contexts without system randomness
though, like block chains.

We'd ideally seperate provers and verifiers using features, but
doing so becomes tricky as crates become more complex, and makes
insecure backends for getrandom tempting.

Instead, we pretend that system randomness exists to satisfy
the compiler, but panic if called without getrandom.  You could
still provide an insecure getrandom, but now you've been warnned
about this footgun.


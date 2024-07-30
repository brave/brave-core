# Cryptographic Protocol

## Notation

We have tried to align notation with that used in the paper
[Privacy Pass: Bypassing Internet Challenges Anonymously](https://www.petsymposium.org/2018/files/papers/issue3/popets-2018-0026.pdf)
sections 3 and 4.

## Description

Let \\(\mathbb{G}\\) be a group (written multiplicatively) of prime order \\(q\\) with generator \\(X\\).
In this implementation we are using Ristretto.

Let \\(\lambda\\) be a security parameter.

The protocol requires three hashing functions, they are assumed to be random oracles:

\\(H_1() \rightarrow \mathbb{G}\\) which hashes to the group, it must ensure the discrete log with respect to other points is unknown.

In this implementation we are using [`RistrettoPoint::from_uniform_bytes`] which uses Elligator2.

\\(H_2() \rightarrow \\{0,1\\}^{\lambda}\\) which hashes to bitstrings.

\\(H_3() \rightarrow \\{0,1\\}^{\lambda}\\) which hashes to bitstrings.

### Setup

The server generates a signing keypair, consisting of a secret key, \\(k\\), which is a random [`Scalar`]
and a commitment to that secret key in the form of a [`PublicKey`].

\\(Y = X^k\\)

### DLEQ Proof

A [`DLEQProof`] seeks to show that for some \\(Y = X^{k_1}\\) and some
\\(Q = P^{k_2}\\) that \\(k_1 = k_2\\).

To do so the prover first generates a random [`Scalar`] \\(t\\) and commits
to it with respect to \\(X\\) and \\(P\\).

\\(A = X^t\\)

\\(B = P^t\\)

The prover then computes \\(c = H_3(X, Y, P, Q, A, B)\\) and \\(s = (t -
ck) \mod q\\).

The [`DLEQProof`] \\((c, s)\\) is then sent to the verifier.

The verifier computes \\(A'\\), \\(B'\\), and \\(c'\\) then verifies \\(c'\\) equals \\(c\\).

\\(A' = X^s \cdot Y^c = X^{t-ck} \cdot X^{ck} = X^t\\)

\\(B' = P^s \cdot Q^c = P^{t-ck} \cdot P^{ck} = P^t\\)

\\(c' = H_3(X, Y, P, Q, A', B')\\)

\\(c' \stackrel{?}{=} c\\)

### Batch Proof

It is possible to construct a [`BatchDLEQProof`] over `m` instantiations
of the original DLEQ proof if \\(X\\) and \\(Y\\) remain constant.

First the prover calculates \\(w\\) which is used to seed a PRNG.

\\(w \leftarrow H(X, Y, \\{ P_i \\} _{i \in m} ,\\{ Q_i \\} _{i \in m })\\)

Next, `m` [`Scalar`]s are sampled from the seeded PRNG.

\\(c_1, \ldots , c_m \leftarrow PRNG(w)\\)

The prover generates the composite elements.

\\(M = P_1^{c_1} \cdot \ldots \cdot P_m^{c_m}\\)

\\(Z = Q_1^{c_1} \cdot \ldots \cdot Q_m^{c_m}\\)

A normal [`DLEQProof`] is then constructed and sent to the user.

\\((c, s) \leftarrow DLEQ_k(X, Y, M, Z)\\)

The verifier recalculates \\(w\\), samples `m` [`Scalar`]s and re-computes \\(M\\) and
\\(Z\\).

Finally the verifier checks the [`DLEQProof`] as described above.

### Signing

The user generates N pairs (referred to as [`Token`]s) which each consist
of a random value and a random blinding factor.

The random value \\(t\\) we'll refer to as a [`TokenPreimage`].
The random blinding factor \\(r\\) is a [`Scalar`].

Given the hash function \\(H_1\\) we can derive a point [`T`].

\\(T = H_1(t)\\)

The user blinds each [`Token`] forming a [`BlindedToken`], point \\(P\\), by performing a scalar multiplication.

\\(P = T^r\\)

The user sends the N [`BlindedToken`]s to the server.

The server signs each [`BlindedToken`] using it's [`SigningKey`], resulting in a [`SignedToken`], point \\(Q\\).

\\(Q = P^k = T^{rk}\\)

The server also creates a [`BatchDLEQProof`] showing all [`BlindedToken`]s were signed
by the same [`SigningKey`] as described above.


The user receives N [`SignedToken`]s from the server as well as the
[`BatchDLEQProof`].

The user verifies the proof and uses the original blinding
factor \\(r\\) from the corresponding [`Token`] to unblind each
[`SignedToken`].

This results in N [`UnblindedToken`]s, each consisting of the unblinded signed point \\(W\\)
and the [`TokenPreimage`], \\(t\\).

\\(W = Q^{1/r} = T^k\\)

### Redemption

At redemption time, the user takes one [`UnblindedToken`] and derives the
shared [`VerificationKey`], \\(K\\).

\\(K = H_2(t, W)\\)

The user uses the shared [`VerificationKey`] to "sign" a message, \\(R\\)
and sends the [`TokenPreimage`] and resulting MAC [`VerificationSignature`] to
the server.

\\(MAC_K(R)\\)

The server re-derives the [`UnblindedToken`] from the [`TokenPreimage`]
and it's [`SigningKey`]. Then it derives the shared [`VerificationKey`]
and checks the included [`VerificationSignature`].

\\(T' = H_1(t)\\)

\\(W' = (T')^k\\)

\\(K' = H_2(t, W')\\)

\\(MAC_K(R) \stackrel{?}{=} MAC_{K'}(R)\\)

If the verification succeeds then the server checks \\(t\\), the [`TokenPreimage`],
to see if it has been previously spent. If not, the redemption succeeds
and it is marked as spent.

[`Scalar`]: https://doc.dalek.rs/curve25519_dalek/scalar/index.html
[`RistrettoPoint::from_uniform_bytes`]: https://doc.dalek.rs/curve25519_dalek/ristretto/struct.RistrettoPoint.html#method.from_uniform_bytes

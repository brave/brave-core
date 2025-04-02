# Commitments

As in Sapling, we require two kinds of commitment schemes in Orchard:
- $\mathit{HomomorphicCommit}$ is a linearly homomorphic commitment scheme with perfect hiding,
  and strong binding reducible to DL.
- $\mathit{Commit}$ and $\mathit{ShortCommit}$ are commitment schemes with perfect hiding, and
  strong binding reducible to DL.

By "strong binding" we mean that the scheme is collision resistant on the input and
randomness.

We instantiate $\mathit{HomomorphicCommit}$ with a Pedersen commitment, and use it for
value commitments:

$$\mathsf{cv} = \mathit{HomomorphicCommit}^{\mathsf{cv}}_{\mathsf{rcv}}(v)$$

We instantiate $\mathit{Commit}$ and $\mathit{ShortCommit}$ with Sinsemilla, and use them
for all other commitments:

$$\mathsf{ivk} = \mathit{ShortCommit}^{\mathsf{ivk}}_{\mathsf{rivk}}(\mathsf{ak}, \mathsf{nk})$$
$$\mathsf{cm} = \mathit{Commit}^{\mathsf{cm}}_{\mathsf{rcm}}(\text{rest of note})$$

This is the same split (and rationale) as in Sapling, but using the more PLONK-efficient
Sinsemilla instead of Bowe--Hopwood Pedersen hashes.

Note that for $\mathsf{ivk}$, we also deviate from Sapling in two ways:

- We use $\mathit{ShortCommit}$ to derive $\mathsf{ivk}$ instead of a full PRF. This removes an
  unnecessary (large) PRF primitive from the circuit, at the cost of requiring $\mathsf{rivk}$ to be
  part of the full viewing key.
- We define $\mathsf{ivk}$ as an integer in $[1, q_P)$; that is, we exclude $\mathsf{ivk} = 0$. For
  Sapling, we relied on BLAKE2s to make $\mathsf{ivk} = 0$ infeasible to produce, but it was still
  technically possible. For Orchard, we get this by construction:
  - $0$ is not a valid x-coordinate for any Pallas point.
  - $\mathsf{SinsemillaShortCommit}$ internally maps points to field elements by replacing the identity (which
    has no affine coordinates) with $0$. But $\mathsf{SinsemillaCommit}$ is defined using incomplete addition, and
    thus will never produce the identity.

# Commitment tree

The commitment tree structure for Orchard is identical to Sapling:

- A single global commitment tree of fixed depth 32.
- Note commitments are appended to the tree in-order from the block.
- Valid Orchard anchors correspond to the global tree state at block boundaries (after all
  commitments from a block have been appended, and before any commitments from the next
  block have been appended).

The only difference is that we instantiate $\mathsf{MerkleCRH}^\mathsf{Orchard}$ with
Sinsemilla (whereas $\mathsf{MerkleCRH}^\mathsf{Sapling}$ used a Bowe--Hopwood Pedersen
hash).

## Uncommitted leaves

The fixed-depth incremental Merkle trees that we use (in Sprout and Sapling, and again in
Orchard) require specifying an "empty" or "uncommitted" leaf - a value that will never be
appended to the tree as a regular leaf.

- For Sprout (and trees composed of the outputs of bit-twiddling hash functions), we use
  the all-zeroes array; the probability of a real note having a colliding note commitment
  is cryptographically negligible.
- For Sapling, where leaves are $u$-coordinates of Jubjub points, we use the value $1$
  which is not the $u$-coordinate of any Jubjub point.

Orchard note commitments are the $x$-coordinates of Pallas points; thus we take the same
approach as Sapling, using a value that is not the $x$-coordinate of any Pallas point as the
uncommitted leaf value. We use the value $2$ for both Pallas and Vesta, because $2^3 + 5$ is
not a square in either $F_p$ or $F_q$:

```python
sage: p = 0x40000000000000000000000000000000224698fc094cf91b992d30ed00000001
sage: q = 0x40000000000000000000000000000000224698fc0994a8dd8c46eb2100000001
sage: EllipticCurve(GF(p), [0, 5]).count_points() == q
True
sage: EllipticCurve(GF(q), [0, 5]).count_points() == p
True
sage: Mod(13, p).is_square()
False
sage: Mod(13, q).is_square()
False
```

> Note: There are also no Pallas points with $x$-coordinate $0$, but we map the identity to
> $(0, 0)$ within the circuit. Although $\mathsf{SinsemillaCommit}$ cannot return the identity
> (the incomplete addition would return $\perp$ instead), it would arguably be confusing to
> rely on that.

## Considered alternatives

We considered splitting the commitment tree into several sub-trees:

- Bundle tree, that accumulates the commitments within a single bundle (and thus a single
  transaction).
- Block tree, that accumulates the bundle tree roots within a single block.
- Global tree, that accumulates the block tree roots.

Each of these trees would have had a fixed depth (necessary for being able to create
proofs). Chains that integrated Orchard could have decoupled the limits on
commitments-per-subtree from higher-layer constraints like block size, by enabling their
blocks and transactions to be structured internally as a series of Orchard blocks or txs
(e.g. a Zcash block would have contained  a `Vec<BlockTreeRoot>`, that each were appended
in-order).

The motivation for considering this change was to improve the lives of light client wallet
developers. When a new note is received, the wallet derives its incremental witness from
the state of the global tree at the point when the note's commitment is appended; this
incremental state then needs to be updated with every subsequent commitment in the block
in-order. Wallets can't get help from the server to create these for new notes without
leaking the specific note that was received.

We decided that this was too large a change from Sapling, and that it should be possible
to improve the Incremental Merkle Tree implementation to work around the efficiency issues
without domain-separating the tree.

# Nullifiers

The nullifier design we use for Orchard is

$$\mathsf{nf} = \mathsf{Extract}_{\mathbb{P}}\big([(F_{\mathsf{nk}}(\rho) + \psi) \bmod{p}] \mathcal{G} + \mathsf{cm}\big),$$

where:

- $F$ is a keyed circuit-efficient PRF (such as Rescue or Poseidon).
- $\rho$ is unique to this output. As with $\mathsf{h_{Sig}}$ in Sprout, $\rho$ includes
  the nullifiers of any Orchard notes being spent in the same action. Given that an action
  consists of a single spend and a single output, we set $\rho$ to be the nullifier of the
  spent note.
- $\psi$ is sender-controlled randomness. It is not required to be unique, and in practice
  is derived from both $\rho$ and a sender-selected random value $\mathsf{rseed}$:
  $$\psi = \mathit{KDF}^\psi(\rho, \mathsf{rseed}).$$
- $\mathcal{G}$ is a fixed independent base.
- $\mathsf{Extract}_{\mathbb{P}}$ extracts the $x$-coordinate of a Pallas curve point.

This gives a note structure of

$$(addr, v, \rho, \psi, \mathsf{rcm}).$$

The note plaintext includes $\mathsf{rseed}$ in place of $\psi$ and $\mathsf{rcm}$, and
omits $\rho$ (which is a public part of the action).

## Security properties

We care about several security properties for our nullifiers:

- **Balance:** can I forge money?

- **Note Privacy:** can I gain information about notes only from the public block chain?

  - This describes notes sent in-band.

- **Note Privacy (OOB):** can I gain information about notes sent out-of-band, only from
  the public block chain?

  - In this case, we assume privacy of the channel over which the note is sent, and that
    the adversary does not have access to any notes sent to the same address which are
    then spent (so that the nullifier is on the block chain somewhere).

- **Spend Unlinkability:** given the incoming viewing key for an address, and not the full
  viewing key, can I (possibly the sender) detect spends of any notes sent to that address?

  - We're giving $\mathsf{ivk}$ to the attacker and allowing it to be the sender in order
    to make this property as strong as possible: they will have *all* the notes sent to that
    address.

- **Faerie Resistance:** can I perform a Faerie Gold attack (i.e. cause notes to be
  accepted that are unspendable)?

  - We're giving the full viewing key to the attacker and allowing it to be the sender in
    order to make this property as strong as possible: they will have *all* the notes sent
    to that address, and be able to derive *every* nullifier.

We assume (and instantiate elsewhere) the following primitives:

- $\mathit{GH}$ is a cryptographic hash into the group (such as BLAKE2s with simplified SWU),
  used to derive all fixed independent bases.
- $E$ is an elliptic curve (such as Pallas).
- $\mathit{KDF}$ is the note encryption key derivation function.

For our chosen design, our desired security properties rely on the following assumptions:

$$
\begin{array}{|l|l|}
\hline
\text{Balance} & \mathit{DL}_E \\
\text{Note Privacy} & \mathit{HashDH}^{\mathit{KDF}}_E \\
\text{Note Privacy (OOB)} & \text{Near perfect} \ddagger \\
\text{Spend Unlinkability} & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F \\
\text{Faerie Resistance} & \mathit{DL}_E \\
\hline
\end{array}
$$

$\mathit{HashDH}^{\mathit{KDF}}_E$ is computational Diffie-Hellman using $\mathit{KDF}$ for
the key derivation, with one-time ephemeral keys. This assumption is heuristically weaker
than $\mathit{DDH}_E$ but stronger than $\mathit{DL}_E$.

We omit $RO_{\mathit{GH}}$ as a security assumption because we only rely on the random oracle
applied to fixed inputs defined by the protocol, i.e. to generate the fixed base
$\mathcal{G}$, not to attacker-specified inputs.

> $\dagger$ We additionally assume that for any input $x$,
> $\{F_{\mathsf{nk}}(x) : \mathsf{nk} \in E\}$ gives a scalar in an adequate range for
> $\mathit{DDH}_E$. (Otherwise, $F$ could be trivial, e.g. independent of $\mathsf{nk}$.)
>
> $\ddagger$ Statistical distance $< 2^{-167.8}$ from perfect.

## Considered alternatives

$\color{red}{\textsf{⚠ Caution}}$: be skeptical of the claims in this table about what
problem(s) each security property depends on. They may not be accurate and are definitely
not fully rigorous.

The entries in this table omit the application of $\mathsf{Extract}_{\mathbb{P}}$,
which is an optimization to halve the nullifier length. That optimization requires its
own security analysis, but because it is a deterministic mapping, only Faerie Resistance
could be affected by it.

$$
\tiny \begin{array}{|c|l|c|c|c|c|c|c|}
\hline\\[-1.5ex]
\mathsf{nf} & \text{Note} & \!\text{Balance}\! & \text{Note Privacy} & \!\text{Note Priv OOB}\! & \!\text{Spend Unlinkability}\! & \text{Faerie Resistance} & \text{Reason not to use} \\[0.6ex]\hline\\[-2.4ex]\hline\\[-1.7ex]
[\mathsf{nk}] [\theta] H & (addr, v, H, \theta, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E & RO_{\mathit{GH}} \wedge \mathit{DL}_E & \text{No SU for DL-breaking}\! \\[0.9ex]\hline\\[-1.7ex]
[\mathsf{nk}] H + [\mathsf{rnf}] \mathcal{I} & (addr, v, H, \mathsf{rnf}, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E & RO_{\mathit{GH}} \wedge \mathit{DL}_E & \text{No SU for DL-breaking}\! \\[0.9ex]\hline\\[-1.7ex]
\mathit{Hash}([\mathsf{nk}] [\theta] H) & (addr, v, H, \theta, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E \vee \mathit{Pre}_{\mathit{Hash}} & \!\mathit{Coll}_{\mathit{Hash}} \wedge RO_{\mathit{GH}} \wedge \mathit{DL}_E\! & \mathit{Coll}_{\mathit{Hash}} \text{ for FR} \\[0.9ex]\hline\\[-1.7ex]
\mathit{Hash}([\mathsf{nk}] H + [\mathsf{rnf}] \mathcal{I}) & (addr, v, H, \mathsf{rnf}, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E \vee \mathit{Pre}_{\mathit{Hash}} & \!\mathit{Coll}_{\mathit{Hash}} \wedge RO_{\mathit{GH}} \wedge \mathit{DL}_E\! & \mathit{Coll}_{\mathit{Hash}} \text{ for FR} \\[0.9ex]\hline\\[-1.7ex]
[F_{\mathsf{nk}}(\psi)] [\theta] H & (addr, v, H, \theta, \psi, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & RO_{\mathit{GH}} \wedge \mathit{DL}_E & \text{Perf. (2 var-base)} \\[0.9ex]\hline\\[-1.7ex]
[F_{\mathsf{nk}}(\psi)] H + [\mathsf{rnf}] \mathcal{I} & (addr, v, H, \mathsf{rnf}, \psi, \mathsf{rcm})\! & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & RO_{\mathit{GH}} \wedge \mathit{DL}_E & \!\text{Perf. (1 var+1 fix-base)}\! \\[0.9ex]\hline\\[-1.7ex]
[F_{\mathsf{nk}}(\psi)] \mathcal{G} + [\theta] H & (addr, v, H, \theta, \psi, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & RO_{\mathit{GH}} \wedge \mathit{DL}_E & \!\text{Perf. (1 var+1 fix-base)}\! \\[0.9ex]\hline\\[-1.7ex]
[F_{\mathsf{nk}}(\psi)] H + \mathsf{cm} & (addr, v, H, \psi, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \mathit{DDH}_E^\dagger & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & RO_{\mathit{GH}} \wedge \mathit{DL}_E & \text{NP(OOB) not perfect} \\[0.9ex]\hline\\[-1.7ex]
[F_{\mathsf{nk}}(\rho, \psi)] \mathcal{G} + \mathsf{cm} & (addr, v, \rho, \psi, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \mathit{DDH}_E^\dagger  & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & \mathit{DL}_E & \text{NP(OOB) not perfect} \\[0.9ex]\hline\\[-1.7ex]
[F_{\mathsf{nk}}(\rho)] \mathcal{G} + \mathsf{cm} & (addr, v, \rho, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \mathit{DDH}_E^\dagger & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & \mathit{DL}_E & \text{NP(OOB) not perfect} \\[0.9ex]\hline\\[-1.7ex]
[F_{\mathsf{nk}}(\rho, \psi)] \mathcal{G_v} + [\mathsf{rnf}] \mathcal{I} & (addr, v, \rho, \mathsf{rnf}, \psi, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & \mathit{Coll}_F \wedge \mathit{DL}_E & \mathit{Coll}_F \text{ for FR} \\[0.9ex]\hline\\[-1.7ex]
[F_{\mathsf{nk}}(\rho)] \mathcal{G_v} + [\mathsf{rnf}] \mathcal{I} & (addr, v, \rho, \mathsf{rnf}, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & \mathit{Coll}_F \wedge \mathit{DL}_E & \mathit{Coll}_F \text{ for FR} \\[0.9ex]\hline\\[-1.7ex]
[(F_{\mathsf{nk}}(\rho) + \psi) \bmod{p}] \mathcal{G_v} & (addr, v, \rho, \psi, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Near perfect} \ddagger & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & \color{red}{\text{broken}} & \text{broken for FR} \\[0.9ex]\hline\\[-1.7ex]
\![F_{\mathsf{nk}}(\rho, \psi)] \mathcal{G} \!+\! \mathit{Commit}^{\mathsf{nf}}_{\mathsf{rnf}}(v, \rho)\! & (addr, v, \rho, \mathsf{rnf}, \psi, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & \mathit{DL}_E & \text{Perf. (2 fix-base)} \\[0.9ex]\hline\\[-1.7ex]
[F_{\mathsf{nk}}(\rho)] \mathcal{G} + \mathit{Commit}^{\mathsf{nf}}_{\mathsf{rnf}}(v, \rho) & (addr, v, \rho, \mathsf{rnf}, \mathsf{rcm}) & \mathit{DL}_E & \mathit{HashDH}^{\mathit{KDF}\!}_E & \text{Perfect} & \mathit{DDH}_E^\dagger \vee \mathit{PRF}_F & \mathit{DL}_E & \text{Perf. (2 fix-base)} \\[0.9ex]\hline
\end{array}
$$

In the above alternatives:

- $\mathit{Hash}$ is a keyed circuit-efficient hash (such as Rescue).
- $\mathcal{I}$ is an fixed independent base, independent of $\mathcal{G}$ and any others
  returned by $\mathit{GH}$.
- $\mathcal{G_v}$ is a pair of fixed independent bases (independent of all others), where
  the specific choice of base depends on whether the note has zero value.
- $H$ is a base unique to this output.

  - For non-zero-valued notes, $H = \mathit{GH}(\rho)$. As with $\mathsf{h_{Sig}}$ in Sprout,
    $\rho$ includes the nullifiers of any Orchard notes being spent in the same action.
  - For zero-valued notes, $H$ is constrained by the circuit to a fixed base independent
    of $\mathcal{I}$ and any others returned by $\mathit{GH}$.

## Rationale

In order to satisfy the **Balance** security property, we require that the circuit must be
able to enforce that only one nullifier is accepted for a given note. As in Sprout and
Sapling, we achieve this by ensuring that the nullifier deterministically depends only on
values committed to (directly or indirectly) by the note commitment. As in Sapling,
this involves arguing that:

- There can be only one $\mathsf{ivk}$ for a given $\mathit{addr}$. This is true because
  the circuit checks that $\mathsf{pk_d} = [\mathsf{ivk}] \mathsf{g_d}$, and the mapping
  $\mathsf{ivk} \mapsto [\mathsf{ivk}] \mathsf{g_d}$ is an injection for any $\mathsf{g_d}$.
  ($\mathsf{ivk}$ is in the base field of $E$, which must be smaller than its scalar field,
  as is the case for Pallas.)
- There can be only one $\mathsf{nk}$ for a given $\mathsf{ivk}$. This is true because the
  circuit checks that $\mathsf{ivk} = \mathit{ShortCommit}^{\mathsf{ivk}}_{\mathsf{rivk}}(\mathsf{ak}, \mathsf{nk})$
  where $\mathit{ShortCommit}$ is binding (see [Commitments](commitments.html)).

### Use of $\rho$

**Faerie Resistance** requires that nullifiers be unique. This is primarily achieved by
taking a unique value (checked for uniqueness by the public consensus rules) as an input
to the nullifier. However, it is also necessary to ensure that the transformations applied
to this value preserve its uniqueness. Meanwhile, to achieve **Spend Unlinkability**, we
require that the nullifier does not reveal any information about the unique value it is
derived from.

The design alternatives fall into two categories in terms of how they balance these
requirements:

- Publish a unique value $\rho$ at note creation time, and blind that value within the
  nullifier computation.

  - This is similar to the approach taken in Sprout and Sapling, which both implemented
    nullifiers as PRF outputs; Sprout uses the compression function from SHA-256, while
    Sapling uses BLAKE2s.

- Derive a unique base $H$ from some unique value, publish that unique base at note
  creation time, and then blind the base (either additively or multiplicatively) during
  nullifier computation.

For **Spend Unlinkability**, the only value unknown to the adversary is $\mathsf{nk}$, and
the cryptographic assumptions only involve the first term (other terms like $\mathsf{cm}$
or $[\mathsf{rnf}] \mathcal{I}$ cannot be extracted directly from the observed nullifiers,
but can be subtracted from them). We therefore ensure that the first term does not commit
directly to the note (to avoid a DL-breaking adversary from immediately breaking **SU**).

We were considering using a design involving $H$ with the goal of eliminating all usages
of a PRF inside the circuit, for two reasons:

- Instantiating $\mathit{PRF}_F$ with a traditional hash function is expensive in the
  circuit.
- We didn't want to solely rely on an algebraic hash function satisfying $\mathit{PRF}_F$
  to achieve **Spend Unlinkability**.

However, those designs rely on both $RO_{\mathit{GH}}$ and $\mathit{DL}_E$ for
**Faerie Resistance**, while still requiring $\mathit{DDH}_E$ for **Spend Unlinkability**.
(There are two designs for which this is not the case, but they rely on
$\mathit{DDH}_E^\dagger$ for **Note Privacy (OOB)** which was not acceptable).

By contrast, several designs involving $\rho$ (including the chosen design) have weaker
assumptions for **Faerie Resistance** (only relying on $\mathit{DL}_E$), and
**Spend Unlinkability** does not require $\mathit{PRF}_F$ to hold: they can fall back
on the same $\mathit{DDH}_E$ assumption as the $H$ designs (along with an additional
assumption about the output of $F$ which is easily satisfied).

### Use of $\psi$

Most of the designs include either a multiplicative blinding term $[\theta] H$, or an
additive blinding term $[\mathsf{rnf}] \mathcal{I}$, in order to achieve perfect
**Note Privacy (OOB)** (to an adversary who does not know the note). The chosen design is
effectively using $[\psi] \mathcal{G}$ for this purpose; a DL-breaking adversary only
learns $F_{\mathsf{nk}}(\rho) + \psi \pmod{p}$. This reduces **Note Privacy (OOB)** from
perfect to statistical, but given that $\psi$ is from a distribution statistically close
to uniform on $[0, q)$, this is statistically close to better than $2^{-128}$. The benefit
is that it does not require an additional scalar multiplication, making it more efficient
inside the circuit.

$\psi$'s derivation has two motivations:

- Deriving from a random value $\mathsf{rseed}$ enables multiple derived values to be
  conveyed to the recipient within an action (such as the ephemeral secret $\mathsf{esk}$,
  per [ZIP 212](https://zips.z.cash/zip-0212)), while keeping the note plaintext short.
- Mixing $\rho$ into the derivation ensures that the sender can't repeat $\psi$ across two
  notes, which could have enabled spend linkability attacks in some designs.

The note that is committed to, and which the circuit takes as input, only includes $\psi$
(i.e. the circuit does not check the derivation from $\mathsf{rseed}$). However, an
adversarial sender is still constrained by this derivation, because the recipient
recomputes $\psi$ during note decryption. If an action were created using an arbitrary
$\psi$ (for which the adversary did not have a corresponding $\mathsf{rseed}$), the
recipient would derive a note commitment that did not match the action's commitment field,
and reject it (as in Sapling).

### Use of $\mathsf{cm}$

The nullifier commits to the note value via $\mathsf{cm}$ for two reasons:

- It domain-separates nullifiers for zero-valued notes from other notes. This is necessary
  because we do not require zero-valued notes to exist in the commitment tree.
- Designs that bind the nullifier to $F_{\mathsf{nk}}(\rho)$ require $\mathit{Coll}_F$ to
  achieve **Faerie Resistance** (and similarly where $\mathit{Hash}$ is applied to a value
  derived from $H$). Adding $\mathsf{cm}$ to the nullifier avoids this assumption: all of
  the bases used to derive $\mathsf{cm}$ are fixed and independent of $\mathcal{G}$, and so
  the nullifier can be viewed as a Pedersen hash where the input includes $\rho$ directly.

The $\mathit{Commit}^{\mathsf{nf}}$ variants were considered to avoid directly depending on
$\mathsf{cm}$ (which in its native type is a base field element, not a group element). We
decided instead to follow Sapling by defining an intermediate representation of
$\mathsf{cm}$ as a group element, that is only used in nullifier computation. The circuit
already needs to compute $\mathsf{cm}$, so this improves performance by removing an
additional commitment calculation from the circuit.

We also considered variants that used a choice of fixed bases $\mathcal{G_v}$ to provide
domain separation for zero-valued notes. The most performant design (similar to the chosen
design) does not achieve **Faerie Resistance** for an adversary that knows the recipient's
full viewing key ($\psi$ could be brute-forced to cancel out $F_{\mathsf{nk}}(\rho)$,
causing a collision), and the other variants require assuming $\mathit{Coll}_F$ as
mentioned above.

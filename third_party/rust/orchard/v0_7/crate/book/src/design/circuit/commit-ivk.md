# $\CommitIvk$

## Message decomposition

$\SinsemillaShortCommit$ is used in the
[$\CommitIvk$ function](https://zips.z.cash/protocol/protocol.pdf#concretesinsemillacommit).
The input to $\SinsemillaShortCommit$ is:

$$\ItoLEBSP{\BaseLength{Orchard}}(\AuthSignPublic) \bconcat \ItoLEBSP{\BaseLength{Orchard}}(\NullifierKey),$$

where $\AuthSignPublic$, $\NullifierKey$ are Pallas base field elements, and $\BaseLength{Orchard} = 255.$

Sinsemilla operates on multiples of 10 bits, so we start by decomposing the message into
chunks:

$$
\begin{align}
\ItoLEBSP{\BaseLength{Orchard}}(\AuthSignPublic) &= a \bconcat b_0 \bconcat b_1 \\
  &= (\text{bits 0..=249 of } \AuthSignPublic) \bconcat
     (\text{bits 250..=253 of } \AuthSignPublic) \bconcat
     (\text{bit 254 of } \AuthSignPublic)  \\
\ItoLEBSP{\BaseLength{Orchard}}(\NullifierKey) &= b_2 \bconcat c \bconcat d_0 \bconcat d_1 \\
  &= (\text{bits 0..=4 of } \NullifierKey) \bconcat
     (\text{bits 5..=244 of } \NullifierKey) \bconcat
     (\text{bits 245..=253 of } \NullifierKey) \bconcat
     (\text{bit 254 of } \NullifierKey) \\
\end{align}
$$

Then we recompose the chunks into message pieces:

$$
\begin{array}{|c|l|}
\hline
\text{Length (bits)} & \text{Piece} \\\hline
250 & a \\
10  & b = b_0 \bconcat b_1 \bconcat b_2 \\
240 & c \\
10  & d = d_0 \bconcat d_1 \\\hline
\end{array}
$$

Each message piece is constrained by $\SinsemillaHash$ to its stated length. Additionally,
$\AuthSignPublic$ and $\NullifierKey$ are witnessed as field elements, so we know they are
canonical. However, we need additional constraints to enforce that:

- The chunks are the correct bit lengths (or else they could overlap in the decompositions
  and allow the prover to witness an arbitrary $\SinsemillaShortCommit$ message).
- The chunks contain the canonical decompositions of $\AuthSignPublic$ and $\NullifierKey$
  (or else the prover could witness an input to $\SinsemillaShortCommit$ that is
  equivalent to $\AuthSignPublic$ and $\NullifierKey$ but not identical).

Some of these constraints can be implemented with reusable circuit gadgets. We define a
custom gate controlled by the selector $q_\CommitIvk$ to hold the remaining constraints.

## Bit length constraints

Chunks $a$ and $c$ are directly constrained by Sinsemilla. For the remaining chunks, we
use the following constraints:

$$
\begin{array}{|c|l|}
\hline
\text{Degree} & \text{Constraint} \\\hline
  & \ShortLookupRangeCheck{b_0, 4} \\\hline
  & \ShortLookupRangeCheck{b_2, 5} \\\hline
  & \ShortLookupRangeCheck{d_0, 9} \\\hline
3 & q_\CommitIvk \cdot \BoolCheck{b_1} = 0 \\\hline
3 & q_\CommitIvk \cdot \BoolCheck{d_1} = 0 \\\hline
\end{array}
$$

where $\BoolCheck{x} = x \cdot (1 - x)$ and $\ShortLookupRangeCheck{}$ is a
[short lookup range check](../decomposition.md#short-range-check).

## Decomposition constraints

We have now derived or witnessed every subpiece, and range-constrained every subpiece:
- $a$ ($250$ bits) is witnessed and constrained outside the gate;
- $b_0$ ($4$ bits) is witnessed and constrained outside the gate;
- $b_1$ ($1$ bits) is witnessed and boolean-constrained in the gate;
- $b_2$ ($5$ bits) is witnessed and constrained outside the gate;
- $c$ ($240$ bits) is witnessed and constrained outside the gate;
- $d_0$ ($9$ bits) is witnessed and constrained outside the gate;
- $d_1$ ($1$ bits) is witnessed and boolean-constrained in the gate.

We can now use them to reconstruct both the (chunked) message pieces, and the original
field element inputs:

$$
\begin{align}
b &= b_0 + 2^4 \cdot b_1 + 2^5 \cdot b_2 \\
d &= d_0 + 2^9 \cdot d_1 \\
\AuthSignPublic &= a + 2^{250} \cdot b_0 + 2^{254} \cdot b_1 \\
\NullifierKey &= b_2 + 2^5 \cdot c + 2^{245} \cdot d_0 + 2^{254} \cdot d_1 \\
\end{align}
$$

$$
\begin{array}{|c|l|}
\hline
\text{Degree} & \text{Constraint} \\\hline
2 & q_\CommitIvk \cdot (b - (b_0 + b_1 \cdot 2^4 + b_2 \cdot 2^5)) = 0 \\\hline
2 & q_\CommitIvk \cdot (d - (d_0 + d_1 \cdot 2^9)) = 0 \\\hline
2 & q_\CommitIvk \cdot (a + b_0 \cdot 2^{250} + b_1 \cdot 2^{254} - \AuthSignPublic) = 0 \\\hline
2 & q_\CommitIvk \cdot (b_2 + c \cdot 2^5 + d_0 \cdot 2^{245} + d_1 \cdot 2^{254} - \NullifierKey) = 0 \\\hline
\end{array}
$$

## Canonicity checks

At this point, we have constrained $\ItoLEBSP{\BaseLength{Orchard}}(\AuthSignPublic)$ and
$\ItoLEBSP{\BaseLength{Orchard}}(\NullifierKey)$ to be 255-bit values, with top bits $b_1$
and $d_1$ respectively. We have also constrained:

$$
\begin{align}
\ItoLEBSP{\BaseLength{Orchard}}(\AuthSignPublic) &= \AuthSignPublic \pmod{q_\mathbb{P}} \\
\ItoLEBSP{\BaseLength{Orchard}}(\NullifierKey) &= \NullifierKey \pmod{q_\mathbb{P}} \\
\end{align}
$$

where $q_\mathbb{P}$ is the Pallas base field modulus. The remaining constraints will
enforce that these are indeed canonically-encoded field elements, i.e.

$$
\begin{align}
\ItoLEBSP{\BaseLength{Orchard}}(\AuthSignPublic) &< q_\mathbb{P} \\
\ItoLEBSP{\BaseLength{Orchard}}(\NullifierKey) &< q_\mathbb{P} \\
\end{align}
$$

The Pallas base field modulus has the form $q_\mathbb{P} = 2^{254} + t_\mathbb{P}$, where
$$t_\mathbb{P} = \mathtt{0x224698fc094cf91b992d30ed00000001}$$
is 126 bits. We therefore know that if the top bit is not set, then the remaining bits
will always comprise a canonical encoding of a field element. Thus the canonicity checks
below are enforced if and only if $b_1 = 1$ (for $\AuthSignPublic$) or $d_1 = 1$ (for
$\NullifierKey$).

> In the constraints below we use a base-$2^{10}$ variant of the method used in libsnark
> (originally from [[SVPBABW2012](https://eprint.iacr.org/2012/598.pdf), Appendix C.1]) for
> range constraints $0 \leq x < t$:
>
> - Let $t'$ be the smallest power of $2^{10}$ greater than $t$.
> - Enforce $0 \leq x < t'$.
> - Let $x' = x + t' - t$.
> - Enforce $0 \leq x' < t'$.

### $\AuthSignPublic$ with $b_1 = 1 \implies \AuthSignPublic \geq 2^{254}$ <a name="canonicity-ak">

In these cases, we check that $\textsf{ak}_{0..=253} < t_\mathbb{P}$:

1. $b_1 = 1 \implies b_0 = 0.$

   Since $b_1 = 1 \implies \AuthSignPublic_{0..=253} < t_\mathbb{P} < 2^{126},$ we know that
   $\AuthSignPublic_{126..=253} = 0,$ and in particular
   $$b_0 := \AuthSignPublic_{250..=253} = 0.$$

2. $b_1 = 1 \implies 0 \leq a < t_\mathbb{P}.$

   To check that $a < t_\mathbb{P}$, we use two constraints:

    a) $0 \leq a < 2^{130}$. This is expressed in the custom gate as
       $$b_1 \cdot z_{a,13} = 0,$$
       where $z_{a,13}$ is the index-13 running sum output by $\SinsemillaHash(a).$

    b) $0 \leq a + 2^{130} - t_\mathbb{P} < 2^{130}$. To check this, we decompose
       $a' = a + 2^{130} - t_\mathbb{P}$ into thirteen 10-bit words (little-endian) using
       a running sum $z_{a'}$, looking up each word in a $10$-bit lookup table. We then
       enforce in the custom gate that
       $$b_1 \cdot z_{a',13} = 0.$$

$$
\begin{array}{|c|l|}
\hline
\text{Degree} & \text{Constraint} \\\hline
3 & q_\CommitIvk \cdot b_1 \cdot b_0 = 0 \\\hline
3 & q_\CommitIvk \cdot b_1 \cdot z_{a,13} = 0 \\\hline
2 & q_\CommitIvk \cdot (a + 2^{130} - t_\mathbb{P} - a') = 0 \\\hline
3 & q_\CommitIvk \cdot b_1 \cdot z_{a',13} = 0 \\\hline
\end{array}
$$

### $\NullifierKey$ with $d_1 = 1 \implies \NullifierKey \geq 2^{254}$ <a name="canonicity-nk">

In these cases, we check that $\textsf{nk}_{0..=253} < t_\mathbb{P}$:

1. $d_1 = 1 \implies d_0 = 0.$

   Since $d_1 = 1 \implies \NullifierKey_{0..=253} < t_\mathbb{P} < 2^{126},$ we know that $\NullifierKey_{126..=253} = 0,$ and in particular $$d_0 := \NullifierKey_{245..=253} = 0.$$

2. $d_1 = 1 \implies 0 \leq b_2 + 2^5 \cdot c < t_\mathbb{P}.$

   To check that $0 \leq b_2 + 2^5 \cdot c < t_\mathbb{P}$, we use two constraints:

    a) $0 \leq b_2 + 2^5 \cdot c < 2^{140}$. $b_2$ is already constrained individually to
       be a $5$-bit value. $z_{c,13}$ is the index-13 running sum output by
       $\SinsemillaHash(c).$ By constraining $$d_1 \cdot z_{c,13} = 0,$$ we constrain
       $b_2 + 2^5 \cdot c < 2^{135} < 2^{140}.$

    b) $0 \leq b_2 + 2^5 \cdot c + 2^{140} - t_\mathbb{P} < 2^{140}$. To check this, we
       decompose ${b_2}c' = b_2 + 2^5 \cdot c + 2^{140} - t_\mathbb{P}$ into fourteen
       10-bit words (little-endian) using a running sum $z_{{b_2}c'}$, looking up each
       word in a $10$-bit lookup table. We then enforce in the custom gate that
       $$d_1 \cdot z_{{b_2}c',14} = 0.$$

$$
\begin{array}{|c|l|}
\hline
\text{Degree} & \text{Constraint} \\\hline
3 & q_\CommitIvk \cdot d_1 \cdot d_0 = 0 \\\hline
3 & q_\CommitIvk \cdot d_1 \cdot z_{c,13} = 0 \\\hline
2 & q_\CommitIvk \cdot (b_2 + c \cdot 2^5 + 2^{140} - t_\mathbb{P} - {b_2}c') = 0 \\\hline
3 & q_\CommitIvk \cdot d_1 \cdot z_{{b_2}c',14} = 0 \\\hline
\end{array}
$$

## Region layout

The constraints controlled by the $q_\CommitIvk$ selector are arranged across 9
advice columns, requiring two rows.

$$
\begin{array}{|c|c|c|c|c|c|c|c|c|c}
                &   &   &     &     &     &          &         &                & q_\CommitIvk \\\hline
\AuthSignPublic & a & b & b_0 & b_1 & b_2 & z_{a,13} & a'      & z_{a',13}      & 1 \\\hline
\NullifierKey   & c & d & d_0 & d_1 &     & z_{c,13} & {b_2}c' & z_{{b_2}c',14} & 0 \\\hline
\end{array}
$$

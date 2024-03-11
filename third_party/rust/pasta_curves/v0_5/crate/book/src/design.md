# Design

## Note on Language

We use slightly different language than others to describe PLONK concepts. Here's the
overview:

1. We like to think of PLONK-like arguments as tables, where each column corresponds to a
   "wire". We refer to entries in this table as "cells".
2. We like to call "selector polynomials" and so on "fixed columns" instead. We then refer
   specifically to a "selector constraint" when a cell in a fixed column is being used to
   control whether a particular constraint is enabled in that row.
3. We call the other polynomials "advice columns" usually, when they're populated by the
   prover.
4. We use the term "rule" to refer to a "gate" like
   $$A(X) \cdot q_A(X) + B(X) \cdot q_B(X) + A(X) \cdot B(X) \cdot q_M(X) + C(X) \cdot q_C(X) = 0.$$
   - TODO: Check how consistent we are with this, and update the code and docs to match.

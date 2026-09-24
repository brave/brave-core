Jubjub supporting evidence
--------------------------

This repository contains supporting evidence that the twisted Edwards curve
-x^2 + y^2 = 1 - (10240/10241).x^2.y^2 of rational points over
GF(52435875175126190479447740508185965837690552500527637822603658699938581184513),
[also called "Jubjub"](https://z.cash/technology/jubjub.html),
satisfies the [SafeCurves criteria](https://safecurves.cr.yp.to/index.html).

The script ``verify.sage`` is based on
[this script from the SafeCurves site](https://safecurves.cr.yp.to/verify.html),
modified

* to support twisted Edwards curves;
* to generate a file 'primes' containing the primes needed for primality proofs,
  if it is not already present;
* to change the directory in which Pocklington proof files are generated
  (``proof/`` rather than ``../../../proof``), and to create that directory
  if it does not exist.

Prerequisites:

* apt-get install sagemath
* pip install sortedcontainers

Run ``sage verify.sage .``, or ``./run.sh`` to also print out the results.

Note that the "rigidity" criterion cannot be checked automatically.

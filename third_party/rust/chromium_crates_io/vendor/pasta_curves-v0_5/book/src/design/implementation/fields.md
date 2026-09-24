# Fields

The [Pasta curves](https://electriccoin.co/blog/the-pasta-curves-for-halo-2-and-beyond/)
are designed to be highly 2-adic, meaning that a large $2^S$
[multiplicative subgroup](https://zcash.github.io/halo2/background/fields.html#multiplicative-subgroups)
exists in each field. That is, we can write $p - 1 \equiv 2^S \cdot T$ with $T$ odd. For
both Pallas and Vesta, $S = 32$; this helps to simplify the field implementations.

## Sarkar square-root algorithm (table-based variant)

We use a technique from [Sarkar2020](https://eprint.iacr.org/2020/1407.pdf) to compute
[square roots](https://zcash.github.io/halo2/background/fields.html#square-roots) in
`pasta_curves`. The intuition
behind the algorithm is that we can split the task into computing square roots in each
multiplicative subgroup.

Suppose we want to find the square root of $u$ modulo one of the Pasta primes $p$, where
$u$ is a non-zero square in $\mathbb{Z}_p^\times$. We define a $2^S$
[root of unity](https://zcash.github.io/halo2/background/fields.html#roots-of-unity)
$g = z^T$ where $z$ is a non-square in $\mathbb{Z}_p^\times$, and precompute the following
tables:

$$
gtab = \begin{bmatrix}
g^0 & g^1 & ... & g^{2^8 - 1} \\
(g^{2^8})^0 & (g^{2^8})^1 & ... & (g^{2^8})^{2^8 - 1} \\
(g^{2^{16}})^0 & (g^{2^{16}})^1 & ... & (g^{2^{16}})^{2^8 - 1} \\
(g^{2^{24}})^0 & (g^{2^{24}})^1 & ... & (g^{2^{24}})^{2^8 - 1}
\end{bmatrix}
$$

$$
invtab = \begin{bmatrix}
(g^{2^{-24}})^0 & (g^{2^{-24}})^1 & ... & (g^{2^{-24}})^{2^8 - 1}
\end{bmatrix}
$$

Let $v = u^{(T-1)/2}$. We can then define $x = uv \cdot v = u^T$ as an element of the
$2^S$ multiplicative subgroup.

Let $x_3 = x, x_2 = x_3^{2^8}, x_1 = x_2^{2^8}, x_0 = x_1^{2^8}.$

### i = 0, 1
Using $invtab$, we lookup $t_0$ such that
$$
x_0 = (g^{2^{-24}})^{t_0} \implies x_0 \cdot g^{t_0 \cdot 2^{24}} = 1.
$$

Define $\alpha_1 = x_1 \cdot (g^{2^{16}})^{t_0}.$

### i = 2
Lookup $t_1$ s.t. 
$$
\begin{array}{ll}
\alpha_1 = (g^{2^{-24}})^{t_1} &\implies x_1 \cdot (g^{2^{16}})^{t_0} = (g^{2^{-24}})^{t_1} \\
&\implies
x_1 \cdot g^{(t_0 + 2^8 \cdot t_1) \cdot 2^{16}} = 1.
\end{array}
$$

Define $\alpha_2 = x_2 \cdot (g^{2^8})^{t_0 + 2^8 \cdot t_1}.$
         
### i = 3
Lookup $t_2$ s.t. 

$$
\begin{array}{ll}
\alpha_2 = (g^{2^{-24}})^{t_2} &\implies x_2 \cdot (g^{2^8})^{t_0 + 2^8\cdot {t_1}} = (g^{2^{-24}})^{t_2} \\
&\implies x_2 \cdot g^{(t_0 + 2^8 \cdot t_1 + 2^{16} \cdot t_2) \cdot 2^8} = 1.
\end{array}
$$

Define $\alpha_3 = x_3 \cdot g^{t_0 + 2^8 \cdot t_1 + 2^{16} \cdot t_2}.$

### Final result
Lookup $t_3$ such that

$$
\begin{array}{ll}
\alpha_3 = (g^{2^{-24}})^{t_3} &\implies x_3 \cdot g^{t_0 + 2^8\cdot {t_1} + 2^{16} \cdot t_2} = (g^{2^{-24}})^{t_3} \\
&\implies x_3 \cdot g^{t_0 + 2^8 \cdot t_1 + 2^{16} \cdot t_2 + 2^{24} \cdot t_3} = 1.
\end{array}
$$

Let $t = t_0 + 2^8 \cdot t_1 + 2^{16} \cdot t_2 + 2^{24} \cdot t_3$.

We can now write
$$
\begin{array}{lclcl}
x_3 \cdot g^{t} = 1 &\implies& x_3 &=& g^{-t} \\
&\implies& uv^2 &=& g^{-t} \\
&\implies& uv &=& v^{-1} \cdot g^{-t} \\
&\implies& uv \cdot g^{t / 2} &=& v^{-1} \cdot g^{-t / 2}.
\end{array}
$$

Squaring the RHS, we observe that $(v^{-1} g^{-t / 2})^2 = v^{-2}g^{-t} = u.$ Therefore,
the square root of $u$ is $uv \cdot g^{t / 2}$; the first part we computed earlier, and
the second part can be computed with three multiplications using lookups in $gtab$.

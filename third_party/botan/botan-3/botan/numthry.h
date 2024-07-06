/*
* Number Theory Functions
* (C) 1999-2007,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_NUMBER_THEORY_H_
#define BOTAN_NUMBER_THEORY_H_

#include <botan/bigint.h>

namespace Botan {

class RandomNumberGenerator;

/**
* Return the absolute value
* @param n an integer
* @return absolute value of n
*/
inline BigInt abs(const BigInt& n) {
   return n.abs();
}

/**
* Compute the greatest common divisor
* @param x a positive integer
* @param y a positive integer
* @return gcd(x,y)
*/
BigInt BOTAN_PUBLIC_API(2, 0) gcd(const BigInt& x, const BigInt& y);

/**
* Least common multiple
* @param x a positive integer
* @param y a positive integer
* @return z, smallest integer such that z % x == 0 and z % y == 0
*/
BigInt BOTAN_PUBLIC_API(2, 0) lcm(const BigInt& x, const BigInt& y);

/**
* @param x an integer
* @return (x*x)
*/
BigInt BOTAN_PUBLIC_API(2, 0) square(const BigInt& x);

/**
* Modular inversion. This algorithm is const time with respect to x,
* as long as x is less than modulus. It also avoids leaking
* information about the modulus, except that it does leak which of 3
* categories the modulus is in: an odd integer, a power of 2, or some
* other even number, and if the modulus is even, leaks the power of 2
* which divides the modulus.
*
* @param x a positive integer
* @param modulus a positive integer
* @return y st (x*y) % modulus == 1 or 0 if no such value
*/
BigInt BOTAN_PUBLIC_API(2, 0) inverse_mod(const BigInt& x, const BigInt& modulus);

/**
* Compute the Jacobi symbol. If n is prime, this is equivalent
* to the Legendre symbol.
* @see http://mathworld.wolfram.com/JacobiSymbol.html
*
* @param a is a non-negative integer
* @param n is an odd integer > 1
* @return (n / m)
*/
int32_t BOTAN_PUBLIC_API(2, 0) jacobi(const BigInt& a, const BigInt& n);

/**
* Modular exponentation
* @param b an integer base
* @param x a positive exponent
* @param m a positive modulus
* @return (b^x) % m
*/
BigInt BOTAN_PUBLIC_API(2, 0) power_mod(const BigInt& b, const BigInt& x, const BigInt& m);

/**
* Compute the square root of x modulo a prime using the Tonelli-Shanks
* algorithm. This algorithm is primarily used for EC point
* decompression which takes only public inputs, as a consequence it is
* not written to be constant-time and may leak side-channel information
* about its arguments.
*
* @param x the input
* @param p the prime modulus
* @return y such that (y*y)%p == x, or -1 if no such integer
*/
BigInt BOTAN_PUBLIC_API(3, 0) sqrt_modulo_prime(const BigInt& x, const BigInt& p);

/**
* @param x an integer
* @return count of the low zero bits in x, or, equivalently, the
*         largest value of n such that 2^n divides x evenly. Returns
*         zero if x is equal to zero.
*/
size_t BOTAN_PUBLIC_API(2, 0) low_zero_bits(const BigInt& x);

/**
* Check for primality
* @param n a positive integer to test for primality
* @param rng a random number generator
* @param prob chance of false positive is bounded by 1/2**prob
* @param is_random true if n was randomly chosen by us
* @return true if all primality tests passed, otherwise false
*/
bool BOTAN_PUBLIC_API(2, 0)
   is_prime(const BigInt& n, RandomNumberGenerator& rng, size_t prob = 64, bool is_random = false);

/**
* Test if the positive integer x is a perfect square ie if there
* exists some positive integer y st y*y == x
* See FIPS 186-4 sec C.4
* @return 0 if the integer is not a perfect square, otherwise
*         returns the positive y st y*y == x
*/
BigInt BOTAN_PUBLIC_API(2, 8) is_perfect_square(const BigInt& x);

/**
* Randomly generate a prime suitable for discrete logarithm parameters
* @param rng a random number generator
* @param bits how large the resulting prime should be in bits
* @param coprime a positive integer that (prime - 1) should be coprime to
* @param equiv a non-negative number that the result should be
               equivalent to modulo equiv_mod
* @param equiv_mod the modulus equiv should be checked against
* @param prob use test so false positive is bounded by 1/2**prob
* @return random prime with the specified criteria
*/
BigInt BOTAN_PUBLIC_API(2, 0) random_prime(RandomNumberGenerator& rng,
                                           size_t bits,
                                           const BigInt& coprime = BigInt::from_u64(0),
                                           size_t equiv = 1,
                                           size_t equiv_mod = 2,
                                           size_t prob = 128);

/**
* Generate a prime suitable for RSA p/q
* @param keygen_rng a random number generator
* @param prime_test_rng a random number generator
* @param bits how large the resulting prime should be in bits (must be >= 512)
* @param coprime a positive integer that (prime - 1) should be coprime to
* @param prob use test so false positive is bounded by 1/2**prob
* @return random prime with the specified criteria
*/
BigInt BOTAN_PUBLIC_API(2, 7) generate_rsa_prime(RandomNumberGenerator& keygen_rng,
                                                 RandomNumberGenerator& prime_test_rng,
                                                 size_t bits,
                                                 const BigInt& coprime,
                                                 size_t prob = 128);

/**
* Return a 'safe' prime, of the form p=2*q+1 with q prime
* @param rng a random number generator
* @param bits is how long the resulting prime should be
* @return prime randomly chosen from safe primes of length bits
*/
BigInt BOTAN_PUBLIC_API(2, 0) random_safe_prime(RandomNumberGenerator& rng, size_t bits);

/**
* The size of the PRIMES[] array
*/
const size_t PRIME_TABLE_SIZE = 6541;

/**
* A const array of all odd primes less than 65535
*/
extern const uint16_t BOTAN_PUBLIC_API(2, 0) PRIMES[];

}  // namespace Botan

#endif

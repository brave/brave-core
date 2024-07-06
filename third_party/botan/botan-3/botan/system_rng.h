/*
* System RNG interface
* (C) 2014,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SYSTEM_RNG_H_
#define BOTAN_SYSTEM_RNG_H_

#include <botan/rng.h>

namespace Botan {

/**
* Return a shared reference to a global PRNG instance provided by the
* operating system. For instance might be instantiated by /dev/urandom
* or CryptGenRandom.
*/
BOTAN_PUBLIC_API(2, 0) RandomNumberGenerator& system_rng();

/*
* Instantiable reference to the system RNG.
*/
class BOTAN_PUBLIC_API(2, 0) System_RNG final : public RandomNumberGenerator {
   public:
      std::string name() const override { return system_rng().name(); }

      bool is_seeded() const override { return system_rng().is_seeded(); }

      bool accepts_input() const override { return system_rng().accepts_input(); }

      void clear() override { system_rng().clear(); }

   protected:
      void fill_bytes_with_input(std::span<uint8_t> out, std::span<const uint8_t> in) override {
         system_rng().randomize_with_input(out, in);
      }
};

}  // namespace Botan

#endif

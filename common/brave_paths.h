#ifndef BRAVE_COMMON_BRAVE_PATHS_H
#define BRAVE_COMMON_BRAVE_PATHS_H

#include "build/build_config.h"

// This file declares path keys for the brave module.  These can be used
// with the PathService to access various special directories and files.

namespace brave {

enum {
  PATH_START = 12000,

  DIR_TEST_DATA,                // Directory where unit test data resides.
  PATH_END
};

// Call once to register the provider for the path keys defined above.
void RegisterPathProvider();

}  // namespace brave

#endif  // BRAVE_COMMON_BRAVE_PATHS_H
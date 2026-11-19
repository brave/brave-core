---
name: brave/endif-comments-by-block-length
description:
  'An #endif closing a block longer than 3 lines, a nested block, or one in code
  that already comments its #endifs carries a comment naming the condition it
  closes; short unambiguous blocks may omit it.'
rubric: readability
severity: nit
---

**Rule.** Give an `#endif` a comment repeating the condition it closes
(`#endif  // BUILDFLAG(ENABLE_SPELLCHECK)`) when the block is longer than 3
lines, when it is inside a nested `#if`, or when the surrounding code already
comments its `#endif`s. The comment may be omitted for a short (1-2 line),
unambiguous block. Do not ask for a comment on such a short block.

**Why.** In long or nested conditionals a bare `#endif` makes it hard to tell
which block it closes. On a one-line block the opening `#if` is right above it,
and the comment adds nothing.

## Examples

Bad:

```cpp
#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#if BUILDFLAG(IS_WIN)
#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"
#endif
#endif
```

Good:

```cpp
#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#if BUILDFLAG(IS_WIN)
#include "brave/components/brave_vpn/common/wireguard/win/storage_utils.h"
#endif  // BUILDFLAG(IS_WIN)
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
```

---
name: brave/json-encode-data-injected-into-javascript
description:
  'C++ data placed into JavaScript that is injected or executed is encoded with
  base::JSONWriter, never concatenated into the script as a string.'
rubric: security
severity: blocking
---

**Rule.** When a script built in C++ contains data (selectors, URLs, strings
from a page, a list or a user), serialise the data with `base::JSONWriter` and
insert the JSON. NEVER concatenate or format the raw values into the script
text, inside quotes or template literals or otherwise.

**Why.** A value containing a quote, a backtick or `</script>` breaks out of the
literal, and lets whoever controls the value run script in the page, possibly in
an isolated world with extra privileges.

## Examples

Bad:

```cpp
std::string script = "const selectors = [`" + selector + "`];";
```

Good:

```cpp
std::string json_selectors;
base::JSONWriter::Write(selectors_list, &json_selectors);
std::string script = "const selectors = " + json_selectors + ";";
```

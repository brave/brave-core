import json
import os
import sys

p = os.path.expanduser(
    "~/Library/Application Support/BraveSoftware/"
    "Brave-Browser-Development/Default/Preferences"
)
if len(sys.argv) > 1:
    p = sys.argv[1]

with open(p) as f:
    d = json.load(f)
d["https_only_mode_enabled"] = True
with open(p, "w") as f:
    json.dump(d, f, indent=3, sort_keys=False)
    f.write("\n")
print("Set https_only_mode_enabled = true in", p)

# Also clear the super_mac in Secure Preferences so Chrome doesn't
# reject the externally-modified pref as tampered.
sp = os.path.join(os.path.dirname(p), "Secure Preferences")
if os.path.exists(sp):
    with open(sp) as f:
        sd = json.load(f)
    if "super_mac" in sd.get("protection", {}):
        del sd["protection"]["super_mac"]
        with open(sp, "w") as f:
            json.dump(sd, f, indent=3, sort_keys=False)
            f.write("\n")
        print("Cleared super_mac in", sp)
    else:
        print("No super_mac to clear")

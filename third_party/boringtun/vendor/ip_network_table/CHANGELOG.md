## Version 0.2.0
* Update `ip_network` crate to 0.4.0

## Version 0.1.2

* Used [fork](https://github.com/JakubOnderka/treebitmap) of treebitmap, that allows to implement new
  methods and fixes bugs.
* Added new methods for `IpNetworkTable`:
  * `matches`
  * `matches_ipv4`
  * `matches_ipv6`
  * `matches_mut`
  * `matches_mut_ipv4`
  * `matches_mut_ipv6`
  * `longest_match_mut`
  * `exact_match_mut`
  * `retain`
  * `iter_ivp4`
  * `iter_ivp6`
  * `iter_mut`
  * `is_empty`
* `IpNetworkTable` now implements [`std::default::Default`](https://doc.rust-lang.org/std/default/trait.Default.html) trait

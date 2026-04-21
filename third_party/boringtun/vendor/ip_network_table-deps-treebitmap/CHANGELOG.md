## Version 0.5

* Added new methods:
  * `exact_match_mut` (fixes #6)
  * `longest_match_mut` (fixes #6)
  * `matches` (#14, thanks @enzious)
  * `matches_mut`
* Panics when IP network address has host bits set when inserting or for `exact_match` also in release mode (#10)
* Fixes undefined behaviour in debug mode (#18, thanks @RalfJung)
* Fixes bug when `longest_match` didn't return correct results (#13, thanks @jiegec)
* Added more tests (thanks @rot256)

#[macro_use]
extern crate getset;

#[derive(CopyGetters, Setters, WithSetters)]
#[getset(get_copy, set, set_with)]
pub struct Plain {
    // If the field was not skipped, the compiler would complain about moving a
    // non-copyable type.
    #[getset(skip)]
    non_copyable: String,

    copyable: usize,
    // Invalid use of skip -- compilation error.
    // #[getset(skip, get_copy)]
    // non_copyable2: String,

    // Invalid use of skip -- compilation error.
    // #[getset(get_copy, skip)]
    // non_copyable2: String,
}

impl Plain {
    fn custom_non_copyable(&self) -> &str {
        &self.non_copyable
    }

    // If the field was not skipped, the compiler would complain about duplicate
    // definitions of `set_non_copyable`.
    fn set_non_copyable(&mut self, val: String) -> &mut Self {
        self.non_copyable = val;
        self
    }

    // If the field was not skipped, the compiler would complain about duplicate
    // definitions of `with_non_copyable`.
    fn with_non_copyable(mut self, val: String) -> Self {
        self.non_copyable = val;
        self
    }
}

impl Default for Plain {
    fn default() -> Self {
        Plain {
            non_copyable: "foo".to_string(),
            copyable: 3,
        }
    }
}

#[test]
fn test_plain() {
    let mut val = Plain::default();
    val.copyable();
    val.custom_non_copyable();
    val.set_non_copyable("bar".to_string());
    val = val.with_non_copyable("foo".to_string());
    let _ = val;
}

use crate::uname::{uname, UnameField};

pub fn get() -> Option<String> {
    uname(UnameField::Machine)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn uname_nonempty() {
        let val = get().expect("architecture::get() failed");
        assert!(!val.is_empty());
    }
}

// linux/ctl_iter.rs

use super::ctl::Ctl;
use ctl_error::SysctlError;
use traits::Sysctl;

/// An iterator over Sysctl entries.
pub struct CtlIter {
    direntries: Vec<walkdir::DirEntry>,
    base: String,
    cur_idx: usize,
}

impl CtlIter {
    /// Return an iterator over the complete sysctl tree.
    pub fn root() -> Self {
        let entries: Vec<walkdir::DirEntry> = walkdir::WalkDir::new("/proc/sys")
            .sort_by(|a, b| a.path().cmp(b.path()))
            .follow_links(false)
            .into_iter()
            .filter_map(|e| e.ok())
            .filter(|e| e.file_type().is_file())
            .collect();
        CtlIter {
            direntries: entries,
            base: "/proc/sys".to_owned(),
            cur_idx: 0,
        }
    }

    /// Return an iterator over all sysctl entries below the given node.
    pub fn below(node: Ctl) -> Self {
        let root = node.path();
        let entries: Vec<walkdir::DirEntry> = walkdir::WalkDir::new(&root)
            .sort_by(|a, b| a.path().cmp(b.path()))
            .follow_links(false)
            .into_iter()
            .filter_map(|e| e.ok())
            .filter(|e| e.file_type().is_file())
            .collect();
        CtlIter {
            direntries: entries,
            base: root,
            cur_idx: 0,
        }
    }
}

impl Iterator for CtlIter {
    type Item = Result<Ctl, SysctlError>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.cur_idx >= self.direntries.len() {
            return None;
        }

        let e: &walkdir::DirEntry = &self.direntries[self.cur_idx];
        self.cur_idx += 1;

        // We continue iterating as long as the oid starts with the base
        if let Some(path) = e.path().to_str() {
            if path.starts_with(&self.base) {
                Some(Ctl::new(path))
            } else {
                None
            }
        } else {
            Some(Err(SysctlError::ParseError))
        }
    }
}

/// Ctl implements the IntoIterator trait to allow for easy iteration
/// over nodes.
///
/// # Example
///
/// ```
/// # use sysctl::Sysctl;
/// #
/// let kern = sysctl::Ctl::new("kernel");
/// for ctl in kern {
///     println!("{}", ctl.name().unwrap());
/// }
/// ```
impl IntoIterator for Ctl {
    type Item = Result<Ctl, SysctlError>;
    type IntoIter = CtlIter;

    fn into_iter(self: Self) -> Self::IntoIter {
        CtlIter::below(self)
    }
}

#[cfg(test)]
mod tests {
    use crate::Sysctl;

    #[test]
    fn ctl_iter_iterate_all() {
        let root = crate::CtlIter::root();
        let all_ctls: Vec<super::Ctl> = root.into_iter().filter_map(Result::ok).collect();
        assert_ne!(all_ctls.len(), 0);
        for ctl in &all_ctls {
            println!("{:?}", ctl.name());
        }
    }

    #[test]
    fn ctl_iter_below_compare_outputs() {
        // NOTE: Some linux distributions require Root permissions
        //        e.g Debian.
        let output = std::process::Command::new("sysctl")
            .arg("user")
            .output()
            .expect("failed to execute process");
        let expected = String::from_utf8_lossy(&output.stdout);

        let node = crate::Ctl::new("user").expect("could not get node");
        let ctls = crate::CtlIter::below(node);
        let mut actual: Vec<String> = vec![];

        for ctl in ctls {
            let ctl = match ctl {
                Err(_) => panic!("ctl error"),
                Ok(s) => s,
            };

            let name = match ctl.name() {
                Ok(s) => s,
                Err(_) => panic!("get ctl name"),
            };

            match ctl.value_type().expect("could not get value type") {
                crate::CtlType::String => {
                    actual.push(format!(
                        "{} = {}",
                        name,
                        ctl.value_string()
                            .expect(&format!("could not get value as string for {}", name))
                    ));
                }
                _ => panic!("sysctl not string type"),
            };
        }
        assert_eq!(actual.join("\n").trim(), expected.trim());
    }
}

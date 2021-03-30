use std::cmp::Ordering;

fn elem_index<T>(v: &Vec<T>, order: Ordering) -> usize
where
    T: std::cmp::Ord,
{
    let mut min_index = 0;
    for i in 1..v.len() {
        let c = &v[i];
        let min = &v[min_index];
        if c.cmp(min) == order {
            min_index = i;
        }
    }
    min_index
}

#[inline]
pub fn min_elem_index<T>(v: &Vec<T>) -> usize
where
    T: std::cmp::Ord,
{
    elem_index(v, Ordering::Less)
}

#[inline]
pub fn max_elem_index<T>(v: &Vec<T>) -> usize
where
    T: std::cmp::Ord,
{
    elem_index(v, Ordering::Greater)
}

pub trait StringUtils {
    fn substring(&self, start: usize, len: usize) -> Self;
}

impl StringUtils for String {
    #[inline]
    fn substring(&self, start: usize, len: usize) -> Self {
        // This method probably isn't very efficent, but we use it with results
        // from find(), which gives us byte offsets. We can't use chars() because
        // that will break for unicode input.
        String::from_utf8(self.as_bytes()[start..len].to_vec()).unwrap_or_default()
    }
}

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

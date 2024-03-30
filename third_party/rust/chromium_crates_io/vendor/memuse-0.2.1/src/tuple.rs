use std::array;

use crate::DynamicUsage;

macro_rules! tuple_impls {
    ($(
        $Tuple:ident {
            $(($idx:tt) -> $T:ident)+
        }
    )+) => {
        $(
            impl<$($T),+> DynamicUsage for ($($T,)+) where $($T: DynamicUsage),+
            {
                fn dynamic_usage(&self) -> usize {
                    array::IntoIter::new([$(self.$idx.dynamic_usage()),+]).sum::<usize>()
                }

                fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
                    array::IntoIter::new([$(self.$idx.dynamic_usage_bounds()),+])
                        .fold(
                            (0, Some(0)),
                            |(acc_lower, acc_upper), (lower, upper)| {
                                (acc_lower + lower, acc_upper.zip(upper).map(|(a, b)| a + b))
                            },
                        )
                }
            }
        )+
    };

    () => {};
}

tuple_impls! {
    Tuple1 {
        (0) -> A
    }
    Tuple2 {
        (0) -> A
        (1) -> B
    }
    Tuple3 {
        (0) -> A
        (1) -> B
        (2) -> C
    }
    Tuple4 {
        (0) -> A
        (1) -> B
        (2) -> C
        (3) -> D
    }
    Tuple5 {
        (0) -> A
        (1) -> B
        (2) -> C
        (3) -> D
        (4) -> E
    }
    Tuple6 {
        (0) -> A
        (1) -> B
        (2) -> C
        (3) -> D
        (4) -> E
        (5) -> F
    }
    Tuple7 {
        (0) -> A
        (1) -> B
        (2) -> C
        (3) -> D
        (4) -> E
        (5) -> F
        (6) -> G
    }
    Tuple8 {
        (0) -> A
        (1) -> B
        (2) -> C
        (3) -> D
        (4) -> E
        (5) -> F
        (6) -> G
        (7) -> H
    }
    Tuple9 {
        (0) -> A
        (1) -> B
        (2) -> C
        (3) -> D
        (4) -> E
        (5) -> F
        (6) -> G
        (7) -> H
        (8) -> I
    }
    Tuple10 {
        (0) -> A
        (1) -> B
        (2) -> C
        (3) -> D
        (4) -> E
        (5) -> F
        (6) -> G
        (7) -> H
        (8) -> I
        (9) -> J
    }
    Tuple11 {
        (0) -> A
        (1) -> B
        (2) -> C
        (3) -> D
        (4) -> E
        (5) -> F
        (6) -> G
        (7) -> H
        (8) -> I
        (9) -> J
        (10) -> K
    }
    Tuple12 {
        (0) -> A
        (1) -> B
        (2) -> C
        (3) -> D
        (4) -> E
        (5) -> F
        (6) -> G
        (7) -> H
        (8) -> I
        (9) -> J
        (10) -> K
        (11) -> L
    }
}

#[cfg(test)]
mod tests {
    use std::collections::HashSet;

    use super::*;
    use crate::hash::WIDTH as HASH_WIDTH;

    #[test]
    fn tuples() {
        let a = (0u8, 0u16, 0u32);
        assert_eq!(a.dynamic_usage(), 0);
        assert_eq!(a.dynamic_usage_bounds(), (0, Some(0)));

        let b = (Vec::<u8>::with_capacity(10), Vec::<u64>::with_capacity(2));
        assert_eq!(b.dynamic_usage(), 26);
        assert_eq!(b.dynamic_usage_bounds(), (26, Some(26)));

        let c = (
            HashSet::<u8>::with_capacity(10),
            Vec::<u64>::with_capacity(2),
        );

        // HashSet has capacity 10 -> 16 buckets
        let lower_bound = 32 + HASH_WIDTH + 16;
        assert_eq!(c.dynamic_usage(), lower_bound);
        assert_eq!(c.dynamic_usage_bounds(), (lower_bound, None));
    }
}

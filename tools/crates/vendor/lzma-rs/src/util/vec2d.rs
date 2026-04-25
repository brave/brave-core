use std::ops::{Index, IndexMut};

/// A 2 dimensional matrix in row-major order backed by a contiguous slice.
#[derive(Debug)]
pub struct Vec2D<T> {
    data: Box<[T]>,
    cols: usize,
}

impl<T> Vec2D<T> {
    /// Initialize a grid of size (`rows`, `cols`) with the given data element.
    pub fn init(data: T, size: (usize, usize)) -> Vec2D<T>
    where
        T: Clone,
    {
        let (rows, cols) = size;
        let len = rows
            .checked_mul(cols)
            .unwrap_or_else(|| panic!("{} rows by {} cols exceeds usize::MAX", rows, cols));
        Vec2D {
            data: vec![data; len].into_boxed_slice(),
            cols,
        }
    }

    /// Fills the grid with elements by cloning `value`.
    pub fn fill(&mut self, value: T)
    where
        T: Clone,
    {
        self.data.fill(value)
    }
}

impl<T> Index<usize> for Vec2D<T> {
    type Output = [T];

    #[inline]
    fn index(&self, row: usize) -> &Self::Output {
        let start_row = row
            .checked_mul(self.cols)
            .unwrap_or_else(|| panic!("{} row by {} cols exceeds usize::MAX", row, self.cols));
        &self.data[start_row..start_row + self.cols]
    }
}

impl<T> IndexMut<usize> for Vec2D<T> {
    #[inline]
    fn index_mut(&mut self, row: usize) -> &mut Self::Output {
        let start_row = row
            .checked_mul(self.cols)
            .unwrap_or_else(|| panic!("{} row by {} cols exceeds usize::MAX", row, self.cols));
        &mut self.data[start_row..start_row + self.cols]
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn init() {
        let vec2d = Vec2D::init(1, (2, 3));
        assert_eq!(vec2d[0], [1, 1, 1]);
        assert_eq!(vec2d[1], [1, 1, 1]);
    }

    #[test]
    #[should_panic]
    fn init_overflow() {
        Vec2D::init(1, (usize::MAX, usize::MAX));
    }

    #[test]
    fn fill() {
        let mut vec2d = Vec2D::init(0, (2, 3));
        vec2d.fill(7);
        assert_eq!(vec2d[0], [7, 7, 7]);
        assert_eq!(vec2d[1], [7, 7, 7]);
    }

    #[test]
    fn index() {
        let vec2d = Vec2D {
            data: vec![0, 1, 2, 3, 4, 5, 6, 7].into_boxed_slice(),
            cols: 2,
        };
        assert_eq!(vec2d[0], [0, 1]);
        assert_eq!(vec2d[1], [2, 3]);
        assert_eq!(vec2d[2], [4, 5]);
        assert_eq!(vec2d[3], [6, 7]);
    }

    #[test]
    fn indexmut() {
        let mut vec2d = Vec2D {
            data: vec![0, 1, 2, 3, 4, 5, 6, 7].into_boxed_slice(),
            cols: 2,
        };

        vec2d[1][1] = 9;
        assert_eq!(vec2d[0], [0, 1]);
        // (1, 1) should be 9.
        assert_eq!(vec2d[1], [2, 9]);
        assert_eq!(vec2d[2], [4, 5]);
        assert_eq!(vec2d[3], [6, 7]);
    }

    #[test]
    #[should_panic]
    fn index_out_of_bounds() {
        let vec2d = Vec2D::init(1, (2, 3));
        let _x = vec2d[2][3];
    }

    #[test]
    #[should_panic]
    fn index_out_of_bounds_vec_edge() {
        let vec2d = Vec2D::init(1, (2, 3));
        let _x = vec2d[1][3];
    }

    #[test]
    #[should_panic]
    fn index_column_out_of_bounds() {
        let vec2d = Vec2D::init(1, (2, 3));
        let _x = vec2d[0][3];
    }

    #[test]
    #[should_panic]
    fn index_row_out_of_bounds() {
        let vec2d = Vec2D::init(1, (2, 3));
        let _x = vec2d[2][0];
    }

    #[test]
    #[should_panic]
    fn index_mul_overflow() {
        // Matrix with 4 columns.
        let matrix = Vec2D::init(0, (3, 4));
        // 2^{usize.numbits() - 2}.
        let row = (usize::MAX / 4) + 1;
        // Returns the same as matrix[0] if overflow is not caught.
        let _ = matrix[row];
    }

    #[test]
    #[should_panic]
    fn index_add_overflow() {
        // Matrix with 5 columns.
        let matrix = Vec2D::init(0, (3, 5));
        // Somehow, as long as numbits(usize) is a multiple of 4, then 5 divides usize::MAX.
        // This is clear in hexadecimal: usize::MAX is 0xFFF...F and usize::MAX / 5 is 0x333...3.
        let row = usize::MAX / 5;
        // This will therefore try to index data[usize::MAX..4].
        let _ = matrix[row];
    }

    #[test]
    #[should_panic]
    fn indexmut_out_of_bounds() {
        let mut vec2d = Vec2D::init(1, (2, 3));
        vec2d[2][3] = 0;
    }

    #[test]
    #[should_panic]
    fn indexmut_out_of_bounds_vec_edge() {
        let mut vec2d = Vec2D::init(1, (2, 3));
        vec2d[1][3] = 0;
    }

    #[test]
    #[should_panic]
    fn indexmut_column_out_of_bounds() {
        let mut vec2d = Vec2D::init(1, (2, 3));
        vec2d[0][3] = 0;
    }

    #[test]
    #[should_panic]
    fn indexmut_row_out_of_bounds() {
        let mut vec2d = Vec2D::init(1, (2, 3));
        vec2d[2][0] = 0;
    }

    #[test]
    #[should_panic]
    fn indexmut_mul_overflow() {
        // Matrix with 4 columns.
        let mut matrix = Vec2D::init(0, (3, 4));
        // 2^{usize.numbits() - 2}.
        let row = (usize::MAX / 4) + 1;
        // Returns the same as matrix[0] if overflow is not caught.
        matrix[row][0] = 9;
    }

    #[test]
    #[should_panic]
    fn indexmut_add_overflow() {
        // Matrix with 5 columns.
        let mut matrix = Vec2D::init(0, (3, 5));
        // Somehow, as long as numbits(usize) is a multiple of 4, then 5 divides usize::MAX.
        // This is clear in hexadecimal: usize::MAX is 0xFFF...F and usize::MAX / 5 is 0x333...3.
        let row = usize::MAX / 5;
        // This will therefore try to index data[usize::MAX..4].
        matrix[row][0] = 9;
    }
}

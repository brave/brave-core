use std::error;
use std::fmt;
use std::io;

use super::TableColumn;
use super::{Any, Column};

/// This is an error that could occur during proving or circuit synthesis.
// TODO: these errors need to be cleaned up
#[non_exhaustive]
#[derive(Debug)]
pub enum Error {
    /// This is an error that can occur during synthesis of the circuit, for
    /// example, when the witness is not present.
    Synthesis,
    /// The provided instances do not match the circuit parameters.
    InvalidInstances,
    /// The constraint system is not satisfied.
    ConstraintSystemFailure,
    /// Out of bounds index passed to a backend
    BoundsFailure,
    /// Opening error
    Opening,
    /// Transcript error
    Transcript(io::Error),
    /// `k` is too small for the given circuit.
    NotEnoughRowsAvailable {
        /// The current value of `k` being used.
        current_k: u32,
    },
    /// Instance provided exceeds number of available rows
    InstanceTooLarge,
    /// Circuit synthesis requires global constants, but circuit configuration did not
    /// call [`ConstraintSystem::enable_constant`] on fixed columns with sufficient space.
    ///
    /// [`ConstraintSystem::enable_constant`]: crate::plonk::ConstraintSystem::enable_constant
    NotEnoughColumnsForConstants,
    /// The instance sets up a copy constraint involving a column that has not been
    /// included in the permutation.
    ColumnNotInPermutation(Column<Any>),
    /// An error relating to a lookup table.
    TableError(TableError),
    /// Try to hash from a private point when allow_init_from_private_point is not set.
    IllegalHashFromPrivatePoint,
}

impl From<io::Error> for Error {
    fn from(error: io::Error) -> Self {
        // The only place we can get io::Error from is the transcript.
        Error::Transcript(error)
    }
}

impl Error {
    /// Constructs an `Error::NotEnoughRowsAvailable`.
    pub(crate) fn not_enough_rows_available(current_k: u32) -> Self {
        Error::NotEnoughRowsAvailable { current_k }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::Synthesis => write!(f, "General synthesis error"),
            Error::InvalidInstances => write!(f, "Provided instances do not match the circuit"),
            Error::ConstraintSystemFailure => write!(f, "The constraint system is not satisfied"),
            Error::BoundsFailure => write!(f, "An out-of-bounds index was passed to the backend"),
            Error::Opening => write!(f, "Multi-opening proof was invalid"),
            Error::Transcript(e) => write!(f, "Transcript error: {}", e),
            Error::NotEnoughRowsAvailable { current_k } => write!(
                f,
                "k = {} is too small for the given circuit. Try using a larger value of k",
                current_k,
            ),
            Error::InstanceTooLarge => write!(f, "Instance vectors are larger than the circuit"),
            Error::NotEnoughColumnsForConstants => {
                write!(
                    f,
                    "Too few fixed columns are enabled for global constants usage"
                )
            }
            Error::ColumnNotInPermutation(column) => write!(
                f,
                "Column {:?} must be included in the permutation. Help: try applying `meta.enable_equalty` on the column",
                column
            ),
            Error::TableError(error) => write!(f, "{}", error),
            Error::IllegalHashFromPrivatePoint =>  write!(
                f,
                "Hashing from private point is disabled"
            )
        }
    }
}

impl error::Error for Error {
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match self {
            Error::Transcript(e) => Some(e),
            _ => None,
        }
    }
}

/// This is an error that could occur during table synthesis.
#[derive(Debug)]
pub enum TableError {
    /// A `TableColumn` has not been assigned.
    ColumnNotAssigned(TableColumn),
    /// A Table has columns of uneven lengths.
    UnevenColumnLengths((TableColumn, usize), (TableColumn, usize)),
    /// Attempt to assign a used `TableColumn`
    UsedColumn(TableColumn),
    /// Attempt to overwrite a default value
    OverwriteDefault(TableColumn, String, String),
}

impl fmt::Display for TableError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            TableError::ColumnNotAssigned(col) => {
                write!(
                    f,
                    "{:?} not fully assigned. Help: assign a value at offset 0.",
                    col
                )
            }
            TableError::UnevenColumnLengths((col, col_len), (table, table_len)) => write!(
                f,
                "{:?} has length {} while {:?} has length {}",
                col, col_len, table, table_len
            ),
            TableError::UsedColumn(col) => {
                write!(f, "{:?} has already been used", col)
            }
            TableError::OverwriteDefault(col, default, val) => {
                write!(
                    f,
                    "Attempted to overwrite default value {} with {} in {:?}",
                    default, val, col
                )
            }
        }
    }
}

//! The Poseidon algebraic hash function.

use std::fmt;
use std::marker::PhantomData;

use group::ff::{Field, PrimeField};
use halo2_proofs::{
    circuit::{AssignedCell, Chip, Layouter},
    plonk::Error,
};

mod pow5;
pub use pow5::{Pow5Chip, Pow5Config, StateWord};

pub use ::halo2_poseidon as primitives;
use primitives::{Absorbing, ConstantLength, Domain, Spec, SpongeMode, Squeezing, State};

/// A word from the padded input to a Poseidon sponge.
#[derive(Clone, Debug)]
pub enum PaddedWord<F: Field> {
    /// A message word provided by the prover.
    Message(AssignedCell<F, F>),
    /// A padding word, that will be fixed in the circuit parameters.
    Padding(F),
}

/// The set of circuit instructions required to use the Poseidon permutation.
pub trait PoseidonInstructions<F: Field, S: Spec<F, T, RATE>, const T: usize, const RATE: usize>:
    Chip<F>
{
    /// Variable representing the word over which the Poseidon permutation operates.
    type Word: Clone + fmt::Debug + From<AssignedCell<F, F>> + Into<AssignedCell<F, F>>;

    /// Applies the Poseidon permutation to the given state.
    fn permute(
        &self,
        layouter: &mut impl Layouter<F>,
        initial_state: &State<Self::Word, T>,
    ) -> Result<State<Self::Word, T>, Error>;
}

/// The set of circuit instructions required to use the [`Sponge`] and [`Hash`] gadgets.
///
/// [`Hash`]: self::Hash
pub trait PoseidonSpongeInstructions<
    F: Field,
    S: Spec<F, T, RATE>,
    D: Domain<F, RATE>,
    const T: usize,
    const RATE: usize,
>: PoseidonInstructions<F, S, T, RATE>
{
    /// Returns the initial empty state for the given domain.
    fn initial_state(&self, layouter: &mut impl Layouter<F>)
        -> Result<State<Self::Word, T>, Error>;

    /// Adds the given input to the state.
    fn add_input(
        &self,
        layouter: &mut impl Layouter<F>,
        initial_state: &State<Self::Word, T>,
        input: &Absorbing<PaddedWord<F>, RATE>,
    ) -> Result<State<Self::Word, T>, Error>;

    /// Extracts sponge output from the given state.
    fn get_output(state: &State<Self::Word, T>) -> Squeezing<Self::Word, RATE>;
}

/// A word over which the Poseidon permutation operates.
#[derive(Debug)]
pub struct Word<
    F: Field,
    PoseidonChip: PoseidonInstructions<F, S, T, RATE>,
    S: Spec<F, T, RATE>,
    const T: usize,
    const RATE: usize,
> {
    inner: PoseidonChip::Word,
}

impl<
        F: Field,
        PoseidonChip: PoseidonInstructions<F, S, T, RATE>,
        S: Spec<F, T, RATE>,
        const T: usize,
        const RATE: usize,
    > Word<F, PoseidonChip, S, T, RATE>
{
    /// The word contained in this gadget.
    pub fn inner(&self) -> PoseidonChip::Word {
        self.inner.clone()
    }

    /// Construct a [`Word`] gadget from the inner word.
    pub fn from_inner(inner: PoseidonChip::Word) -> Self {
        Self { inner }
    }
}

fn poseidon_sponge<
    F: Field,
    PoseidonChip: PoseidonSpongeInstructions<F, S, D, T, RATE>,
    S: Spec<F, T, RATE>,
    D: Domain<F, RATE>,
    const T: usize,
    const RATE: usize,
>(
    chip: &PoseidonChip,
    mut layouter: impl Layouter<F>,
    state: &mut State<PoseidonChip::Word, T>,
    input: Option<&Absorbing<PaddedWord<F>, RATE>>,
) -> Result<Squeezing<PoseidonChip::Word, RATE>, Error> {
    if let Some(input) = input {
        *state = chip.add_input(&mut layouter, state, input)?;
    }
    *state = chip.permute(&mut layouter, state)?;
    Ok(PoseidonChip::get_output(state))
}

/// A Poseidon sponge.
#[derive(Debug)]
pub struct Sponge<
    F: Field,
    PoseidonChip: PoseidonSpongeInstructions<F, S, D, T, RATE>,
    S: Spec<F, T, RATE>,
    M: SpongeMode,
    D: Domain<F, RATE>,
    const T: usize,
    const RATE: usize,
> {
    chip: PoseidonChip,
    mode: M,
    state: State<PoseidonChip::Word, T>,
    _marker: PhantomData<D>,
}

impl<
        F: Field,
        PoseidonChip: PoseidonSpongeInstructions<F, S, D, T, RATE>,
        S: Spec<F, T, RATE>,
        D: Domain<F, RATE>,
        const T: usize,
        const RATE: usize,
    > Sponge<F, PoseidonChip, S, Absorbing<PaddedWord<F>, RATE>, D, T, RATE>
{
    /// Constructs a new duplex sponge for the given Poseidon specification.
    pub fn new(chip: PoseidonChip, mut layouter: impl Layouter<F>) -> Result<Self, Error> {
        chip.initial_state(&mut layouter).map(|state| Sponge {
            chip,
            mode: Absorbing::init_empty(),
            state,
            _marker: PhantomData::default(),
        })
    }

    /// Absorbs an element into the sponge.
    pub fn absorb(
        &mut self,
        mut layouter: impl Layouter<F>,
        value: PaddedWord<F>,
    ) -> Result<(), Error> {
        let value = match self.mode.absorb(value) {
            Ok(()) => return Ok(()),
            Err(value) => value,
        };

        // We've already absorbed as many elements as we can
        let _ = poseidon_sponge(
            &self.chip,
            layouter.namespace(|| "PoseidonSponge"),
            &mut self.state,
            Some(&self.mode),
        )?;
        self.mode = Absorbing::init_empty();
        self.mode.absorb(value).expect("state is not full");

        Ok(())
    }

    /// Transitions the sponge into its squeezing state.
    #[allow(clippy::type_complexity)]
    pub fn finish_absorbing(
        mut self,
        mut layouter: impl Layouter<F>,
    ) -> Result<Sponge<F, PoseidonChip, S, Squeezing<PoseidonChip::Word, RATE>, D, T, RATE>, Error>
    {
        let mode = poseidon_sponge(
            &self.chip,
            layouter.namespace(|| "PoseidonSponge"),
            &mut self.state,
            Some(&self.mode),
        )?;

        Ok(Sponge {
            chip: self.chip,
            mode,
            state: self.state,
            _marker: PhantomData::default(),
        })
    }
}

impl<
        F: Field,
        PoseidonChip: PoseidonSpongeInstructions<F, S, D, T, RATE>,
        S: Spec<F, T, RATE>,
        D: Domain<F, RATE>,
        const T: usize,
        const RATE: usize,
    > Sponge<F, PoseidonChip, S, Squeezing<PoseidonChip::Word, RATE>, D, T, RATE>
{
    /// Squeezes an element from the sponge.
    pub fn squeeze(&mut self, mut layouter: impl Layouter<F>) -> Result<AssignedCell<F, F>, Error> {
        loop {
            if let Some(value) = self.mode.squeeze() {
                return Ok(value.into());
            }

            // We've already squeezed out all available elements
            self.mode = poseidon_sponge(
                &self.chip,
                layouter.namespace(|| "PoseidonSponge"),
                &mut self.state,
                None,
            )?;
        }
    }
}

/// A Poseidon hash function, built around a sponge.
#[derive(Debug)]
pub struct Hash<
    F: Field,
    PoseidonChip: PoseidonSpongeInstructions<F, S, D, T, RATE>,
    S: Spec<F, T, RATE>,
    D: Domain<F, RATE>,
    const T: usize,
    const RATE: usize,
> {
    sponge: Sponge<F, PoseidonChip, S, Absorbing<PaddedWord<F>, RATE>, D, T, RATE>,
}

impl<
        F: Field,
        PoseidonChip: PoseidonSpongeInstructions<F, S, D, T, RATE>,
        S: Spec<F, T, RATE>,
        D: Domain<F, RATE>,
        const T: usize,
        const RATE: usize,
    > Hash<F, PoseidonChip, S, D, T, RATE>
{
    /// Initializes a new hasher.
    pub fn init(chip: PoseidonChip, layouter: impl Layouter<F>) -> Result<Self, Error> {
        Sponge::new(chip, layouter).map(|sponge| Hash { sponge })
    }
}

impl<
        F: PrimeField,
        PoseidonChip: PoseidonSpongeInstructions<F, S, ConstantLength<L>, T, RATE>,
        S: Spec<F, T, RATE>,
        const T: usize,
        const RATE: usize,
        const L: usize,
    > Hash<F, PoseidonChip, S, ConstantLength<L>, T, RATE>
{
    /// Hashes the given input.
    pub fn hash(
        mut self,
        mut layouter: impl Layouter<F>,
        message: [AssignedCell<F, F>; L],
    ) -> Result<AssignedCell<F, F>, Error> {
        for (i, value) in message
            .into_iter()
            .map(PaddedWord::Message)
            .chain(<ConstantLength<L> as Domain<F, RATE>>::padding(L).map(PaddedWord::Padding))
            .enumerate()
        {
            self.sponge
                .absorb(layouter.namespace(|| format!("absorb_{}", i)), value)?;
        }
        self.sponge
            .finish_absorbing(layouter.namespace(|| "finish absorbing"))?
            .squeeze(layouter.namespace(|| "squeeze"))
    }
}

use crate::decode::lzbuffer::{LzBuffer, LzCircularBuffer};
use crate::decode::lzma::{DecoderState, LzmaParams};
use crate::decode::rangecoder::RangeDecoder;
use crate::decompress::Options;
use crate::error::Error;
use std::fmt::Debug;
use std::io::{self, BufRead, Cursor, Read, Write};

/// Minimum header length to be read.
/// - props: u8 (1 byte)
/// - dict_size: u32 (4 bytes)
const MIN_HEADER_LEN: usize = 5;

/// Max header length to be read.
/// - unpacked_size: u64 (8 bytes)
const MAX_HEADER_LEN: usize = MIN_HEADER_LEN + 8;

/// Required bytes after the header.
/// - ignore: u8 (1 byte)
/// - code: u32 (4 bytes)
const START_BYTES: usize = 5;

/// Maximum number of bytes to buffer while reading the header.
const MAX_TMP_LEN: usize = MAX_HEADER_LEN + START_BYTES;

/// Internal state of this streaming decoder. This is needed because we have to
/// initialize the stream before processing any data.
#[derive(Debug)]
enum State<W>
where
    W: Write,
{
    /// Stream is initialized but header values have not yet been read.
    Header(W),
    /// Header values have been read and the stream is ready to process more data.
    Data(RunState<W>),
}

/// Structures needed while decoding data.
struct RunState<W>
where
    W: Write,
{
    decoder: DecoderState,
    range: u32,
    code: u32,
    output: LzCircularBuffer<W>,
}

impl<W> Debug for RunState<W>
where
    W: Write,
{
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        fmt.debug_struct("RunState")
            .field("range", &self.range)
            .field("code", &self.code)
            .finish()
    }
}

/// Lzma decompressor that can process multiple chunks of data using the
/// `io::Write` interface.
#[cfg_attr(docsrs, doc(cfg(stream)))]
pub struct Stream<W>
where
    W: Write,
{
    /// Temporary buffer to hold data while the header is being read.
    tmp: Cursor<[u8; MAX_TMP_LEN]>,
    /// Whether the stream is initialized and ready to process data.
    /// An `Option` is used to avoid interior mutability when updating the state.
    state: Option<State<W>>,
    /// Options given when a stream is created.
    options: Options,
}

impl<W> Stream<W>
where
    W: Write,
{
    /// Initialize the stream. This will consume the `output` which is the sink
    /// implementing `io::Write` that will receive decompressed bytes.
    pub fn new(output: W) -> Self {
        Self::new_with_options(&Options::default(), output)
    }

    /// Initialize the stream with the given `options`. This will consume the
    /// `output` which is the sink implementing `io::Write` that will
    /// receive decompressed bytes.
    pub fn new_with_options(options: &Options, output: W) -> Self {
        Self {
            tmp: Cursor::new([0; MAX_TMP_LEN]),
            state: Some(State::Header(output)),
            options: *options,
        }
    }

    /// Get a reference to the output sink
    pub fn get_output(&self) -> Option<&W> {
        self.state.as_ref().map(|state| match state {
            State::Header(output) => &output,
            State::Data(state) => state.output.get_output(),
        })
    }

    /// Get a mutable reference to the output sink
    pub fn get_output_mut(&mut self) -> Option<&mut W> {
        self.state.as_mut().map(|state| match state {
            State::Header(output) => output,
            State::Data(state) => state.output.get_output_mut(),
        })
    }

    /// Consumes the stream and returns the output sink. This also makes sure
    /// we have properly reached the end of the stream.
    pub fn finish(mut self) -> crate::error::Result<W> {
        if let Some(state) = self.state.take() {
            match state {
                State::Header(output) => {
                    if self.tmp.position() > 0 {
                        Err(Error::LzmaError("failed to read header".to_string()))
                    } else {
                        Ok(output)
                    }
                }
                State::Data(mut state) => {
                    if !self.options.allow_incomplete {
                        // Process one last time with empty input to force end of
                        // stream checks
                        let mut stream =
                            Cursor::new(&self.tmp.get_ref()[0..self.tmp.position() as usize]);
                        let mut range_decoder =
                            RangeDecoder::from_parts(&mut stream, state.range, state.code);
                        state
                            .decoder
                            .process(&mut state.output, &mut range_decoder)?;
                    }
                    let output = state.output.finish()?;
                    Ok(output)
                }
            }
        } else {
            // this will occur if a call to `write()` fails
            Err(Error::LzmaError(
                "can't finish stream because of previous write error".to_string(),
            ))
        }
    }

    /// Attempts to read the header and transition into a running state.
    ///
    /// This function will consume the state, returning the next state on both
    /// error and success.
    fn read_header<R: BufRead>(
        output: W,
        mut input: &mut R,
        options: &Options,
    ) -> crate::error::Result<State<W>> {
        match LzmaParams::read_header(&mut input, options) {
            Ok(params) => {
                let decoder = DecoderState::new(params.properties, params.unpacked_size);
                let output = LzCircularBuffer::from_stream(
                    output,
                    params.dict_size as usize,
                    options.memlimit.unwrap_or(usize::MAX),
                );
                // The RangeDecoder is only kept temporarily as we are processing
                // chunks of data.
                if let Ok(rangecoder) = RangeDecoder::new(&mut input) {
                    Ok(State::Data(RunState {
                        decoder,
                        output,
                        range: rangecoder.range,
                        code: rangecoder.code,
                    }))
                } else {
                    // Failed to create a RangeDecoder because we need more data,
                    // try again later.
                    Ok(State::Header(output.into_output()))
                }
            }
            // Failed to read_header() because we need more data, try again later.
            Err(Error::HeaderTooShort(_)) => Ok(State::Header(output)),
            // Fatal error. Don't retry.
            Err(e) => Err(e),
        }
    }

    /// Process compressed data
    fn read_data<R: BufRead>(mut state: RunState<W>, mut input: &mut R) -> io::Result<RunState<W>> {
        // Construct our RangeDecoder from the previous range and code
        // values.
        let mut rangecoder = RangeDecoder::from_parts(&mut input, state.range, state.code);

        // Try to process all bytes of data.
        state
            .decoder
            .process_stream(&mut state.output, &mut rangecoder)
            .map_err(|e| -> io::Error { e.into() })?;

        Ok(RunState {
            decoder: state.decoder,
            output: state.output,
            range: rangecoder.range,
            code: rangecoder.code,
        })
    }
}

impl<W> Debug for Stream<W>
where
    W: Write + Debug,
{
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        fmt.debug_struct("Stream")
            .field("tmp", &self.tmp.position())
            .field("state", &self.state)
            .field("options", &self.options)
            .finish()
    }
}

impl<W> Write for Stream<W>
where
    W: Write,
{
    fn write(&mut self, data: &[u8]) -> io::Result<usize> {
        let mut input = Cursor::new(data);

        if let Some(state) = self.state.take() {
            let state = match state {
                // Read the header values and transition into a running state.
                State::Header(state) => {
                    let res = if self.tmp.position() > 0 {
                        // attempt to fill the tmp buffer
                        let position = self.tmp.position();
                        let bytes_read =
                            input.read(&mut self.tmp.get_mut()[position as usize..])?;
                        let bytes_read = if bytes_read < std::u64::MAX as usize {
                            bytes_read as u64
                        } else {
                            return Err(io::Error::new(
                                io::ErrorKind::Other,
                                "Failed to convert integer to u64.",
                            ));
                        };
                        self.tmp.set_position(position + bytes_read);

                        // attempt to read the header from our tmp buffer
                        let (position, res) = {
                            let mut tmp_input =
                                Cursor::new(&self.tmp.get_ref()[0..self.tmp.position() as usize]);
                            let res = Stream::read_header(state, &mut tmp_input, &self.options);
                            (tmp_input.position(), res)
                        };

                        // discard all bytes up to position if reading the header
                        // was successful
                        if let Ok(State::Data(_)) = &res {
                            let tmp = *self.tmp.get_ref();
                            let end = self.tmp.position();
                            let new_len = end - position;
                            (&mut self.tmp.get_mut()[0..new_len as usize])
                                .copy_from_slice(&tmp[position as usize..end as usize]);
                            self.tmp.set_position(new_len);
                        }
                        res
                    } else {
                        Stream::read_header(state, &mut input, &self.options)
                    };

                    match res {
                        // occurs when not enough input bytes were provided to
                        // read the entire header
                        Ok(State::Header(val)) => {
                            if self.tmp.position() == 0 {
                                // reset the cursor because we may have partial reads
                                input.set_position(0);
                                let bytes_read = input.read(&mut self.tmp.get_mut()[..])?;
                                let bytes_read = if bytes_read < std::u64::MAX as usize {
                                    bytes_read as u64
                                } else {
                                    return Err(io::Error::new(
                                        io::ErrorKind::Other,
                                        "Failed to convert integer to u64.",
                                    ));
                                };
                                self.tmp.set_position(bytes_read);
                            }
                            State::Header(val)
                        }

                        // occurs when the header was successfully read and we
                        // move on to the next state
                        Ok(State::Data(val)) => State::Data(val),

                        // occurs when the output was consumed due to a
                        // non-recoverable error
                        Err(e) => {
                            return Err(match e {
                                Error::IoError(e) | Error::HeaderTooShort(e) => e,
                                Error::LzmaError(e) | Error::XzError(e) => {
                                    io::Error::new(io::ErrorKind::Other, e)
                                }
                            });
                        }
                    }
                }

                // Process another chunk of data.
                State::Data(state) => {
                    let state = if self.tmp.position() > 0 {
                        let mut tmp_input =
                            Cursor::new(&self.tmp.get_ref()[0..self.tmp.position() as usize]);
                        let res = Stream::read_data(state, &mut tmp_input)?;
                        self.tmp.set_position(0);
                        res
                    } else {
                        state
                    };
                    State::Data(Stream::read_data(state, &mut input)?)
                }
            };
            self.state.replace(state);
        }
        Ok(input.position() as usize)
    }

    /// Flushes the output sink. The internal buffer isn't flushed to avoid
    /// corrupting the internal state. Instead, call `finish()` to finalize the
    /// stream and flush all remaining internal data.
    fn flush(&mut self) -> io::Result<()> {
        if let Some(ref mut state) = self.state {
            match state {
                State::Header(_) => Ok(()),
                State::Data(state) => state.output.get_output_mut().flush(),
            }
        } else {
            Ok(())
        }
    }
}

impl From<Error> for io::Error {
    fn from(error: Error) -> io::Error {
        io::Error::new(io::ErrorKind::Other, format!("{:?}", error))
    }
}

#[cfg(test)]
mod test {
    use super::*;

    /// Test an empty stream
    #[test]
    fn test_stream_noop() {
        let stream = Stream::new(Vec::new());
        assert!(stream.get_output().unwrap().is_empty());

        let output = stream.finish().unwrap();
        assert!(output.is_empty());
    }

    /// Test writing an empty slice
    #[test]
    fn test_stream_zero() {
        let mut stream = Stream::new(Vec::new());

        stream.write_all(&[]).unwrap();
        stream.write_all(&[]).unwrap();

        let output = stream.finish().unwrap();

        assert!(output.is_empty());
    }

    /// Test a bad header value
    #[test]
    #[should_panic(expected = "LZMA header invalid properties: 255 must be < 225")]
    fn test_bad_header() {
        let input = [255u8; 32];

        let mut stream = Stream::new(Vec::new());

        stream.write_all(&input[..]).unwrap();

        let output = stream.finish().unwrap();

        assert!(output.is_empty());
    }

    /// Test processing only partial data
    #[test]
    fn test_stream_incomplete() {
        let input = b"\x5d\x00\x00\x80\x00\xff\xff\xff\xff\xff\xff\xff\xff\x00\x83\xff\
                      \xfb\xff\xff\xc0\x00\x00\x00";
        // Process until this index is reached.
        let mut end = 1u64;

        // Test when we fail to provide the minimum number of bytes required to
        // read the header. Header size is 13 bytes but we also read the first 5
        // bytes of data.
        while end < (MAX_HEADER_LEN + START_BYTES) as u64 {
            let mut stream = Stream::new(Vec::new());
            stream.write_all(&input[..end as usize]).unwrap();
            assert_eq!(stream.tmp.position(), end);

            let err = stream.finish().unwrap_err();
            assert!(
                err.to_string().contains("failed to read header"),
                "error was: {}",
                err
            );

            end += 1;
        }

        // Test when we fail to provide enough bytes to terminate the stream. A
        // properly terminated stream will have a code value of 0.
        while end < input.len() as u64 {
            let mut stream = Stream::new(Vec::new());
            stream.write_all(&input[..end as usize]).unwrap();

            // Header bytes will be buffered until there are enough to read
            if end < (MAX_HEADER_LEN + START_BYTES) as u64 {
                assert_eq!(stream.tmp.position(), end);
            }

            let err = stream.finish().unwrap_err();
            assert!(err.to_string().contains("failed to fill whole buffer"));

            end += 1;
        }
    }

    /// Test processing all chunk sizes
    #[test]
    fn test_stream_chunked() {
        let small_input = include_bytes!("../../tests/files/small.txt");

        let mut reader = io::Cursor::new(&small_input[..]);
        let mut small_input_compressed = Vec::new();
        crate::lzma_compress(&mut reader, &mut small_input_compressed).unwrap();

        let input : Vec<(&[u8], &[u8])> = vec![
            (b"\x5d\x00\x00\x80\x00\xff\xff\xff\xff\xff\xff\xff\xff\x00\x83\xff\xfb\xff\xff\xc0\x00\x00\x00", b""),
            (&small_input_compressed[..], small_input)];
        for (input, expected) in input {
            for chunk in 1..input.len() {
                let mut consumed = 0;
                let mut stream = Stream::new(Vec::new());
                while consumed < input.len() {
                    let end = std::cmp::min(consumed + chunk, input.len());
                    stream.write_all(&input[consumed..end]).unwrap();
                    consumed = end;
                }
                let output = stream.finish().unwrap();
                assert_eq!(expected, &output[..]);
            }
        }
    }

    #[test]
    fn test_stream_corrupted() {
        let mut stream = Stream::new(Vec::new());
        let err = stream
            .write_all(b"corrupted bytes here corrupted bytes here")
            .unwrap_err();
        assert!(err.to_string().contains("beyond output size"));
        let err = stream.finish().unwrap_err();
        assert!(err
            .to_string()
            .contains("can\'t finish stream because of previous write error"));
    }

    #[test]
    fn test_allow_incomplete() {
        let input = include_bytes!("../../tests/files/small.txt");

        let mut reader = io::Cursor::new(&input[..]);
        let mut compressed = Vec::new();
        crate::lzma_compress(&mut reader, &mut compressed).unwrap();
        let compressed = &compressed[..compressed.len() / 2];

        // Should fail to finish() without the allow_incomplete option.
        let mut stream = Stream::new(Vec::new());
        stream.write_all(&compressed[..]).unwrap();
        stream.finish().unwrap_err();

        // Should succeed with the allow_incomplete option.
        let mut stream = Stream::new_with_options(
            &Options {
                allow_incomplete: true,
                ..Default::default()
            },
            Vec::new(),
        );
        stream.write_all(&compressed[..]).unwrap();
        let output = stream.finish().unwrap();
        assert_eq!(output, &input[..26]);
    }
}

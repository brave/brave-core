#include <iostream>
#include <sstream>
#include <streambuf>
#include <cstdint>

/********************************************//**
* @brief An unbuffered `std::streambuf` CRTP base class for handling streaming & syncing of data.
*        All derived classes MUST implement the read, write, seek, tell, and flush functions.
*
* @param stream_type - Type of the derived class (CRTP derived).
* @param char_type - Type of buffer. char for example.
* @param traits_type - Type traits for the char_type.
***********************************************/
template<typename stream_type, typename char_type, typename traits_type = std::char_traits<char_type>>
class StreamBuffer : public std::basic_streambuf<char_type, traits_type>
{
protected:
    using parent_type = std::basic_streambuf<char_type, traits_type>;
    using int_type = typename parent_type::int_type;
    using pos_type = typename parent_type::pos_type;
    using off_type = typename parent_type::off_type;
    using seekdir = typename std::ios_base::seekdir;
    using openmode = typename std::ios_base::openmode;
    
private:
    /// An std::string that holds the log in memory for a brief moment until it is synced (immediately).
    std::basic_string<char_type, traits_type, std::allocator<char_type>> pBuffer;
    
    /// Implementing the `std::streambuf` interface.
    int sync();
    int_type underflow();
    int_type overflow(int_type c  = traits_type::eof());
    pos_type seekoff(off_type pos, seekdir dir, openmode mode = parent_type::in | parent_type::out);
    pos_type seekpos(pos_type pos, openmode mode = parent_type::in | parent_type::out);
    
public:
    /// Constructs an `std::streambuf` with no buffering and no initial pointers.
    StreamBuffer();
    virtual ~StreamBuffer();
    
    StreamBuffer(const StreamBuffer& other) = delete;
    StreamBuffer& operator = (const StreamBuffer& other) = delete;
};

template<typename stream_type, typename char_type, typename traits_type>
StreamBuffer<stream_type, char_type, traits_type>::StreamBuffer() : pBuffer()
{
    /// No buffering.
    /// Could also use pbufset instead, but this is more clear that there's no buffering for reading AND writing.
    parent_type::setg(nullptr, nullptr, nullptr);
    parent_type::setp(nullptr, nullptr);
}

template<typename stream_type, typename char_type, typename traits_type>
StreamBuffer<stream_type, char_type, traits_type>::~StreamBuffer()
{
    //Child class will handle destruction and syncing.
    //sync();
}

template<typename stream_type, typename char_type, typename traits_type>
int StreamBuffer<stream_type, char_type, traits_type>::sync()
{
    /// If we are able to write our entire buffer, then we can clear the internal buffer and flush the streambuf.
    if (static_cast<stream_type*>(this)->write(&pBuffer[0], pBuffer.size(), sizeof(char_type)) == pBuffer.size())
    {
        pBuffer.clear();
        static_cast<stream_type*>(this)->flush();
        return 0;
    }
    
    /// End of file - Nothing to write/flush
    return traits_type::eof();
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::int_type StreamBuffer<stream_type, char_type, traits_type>::underflow()
{
    /// In our case, we do not have any get-pointer and end-get-pointer
    if (parent_type::gptr() < parent_type::egptr())
    {
        /// But for future we can.. so let's return the char at the current pointer's location.
        return traits_type::to_int_type(*parent_type::gptr());
    }
    else
    {
        /// Calculate the offset from the putback pointer and its base
        std::size_t pOffset = std::abs(parent_type::pptr() - parent_type::pbase());
        
        /// Calculate the offset from the get pointer and its base
        std::size_t gOffset = std::abs(parent_type::gptr() - parent_type::eback());
        
        /// Append a null character to our buffer to make calculations easier
        pBuffer.push_back(char_type());
        
        /// Update the get pointer and putback pointers
        parent_type::setg(&pBuffer[0], &pBuffer[0] + gOffset, &pBuffer[0] + pBuffer.size());
        parent_type::setp(&pBuffer[0], &pBuffer[0] + pBuffer.size());
        parent_type::pbump(static_cast<int>(pOffset)); /// Bump the putback by the offset it was previously
    }
    
    /// Check if we have an underflow and move the data
    char_type* offset = &pBuffer[0];
    if (parent_type::eback() == &pBuffer[0])
    {
        std::memmove(&pBuffer[0], parent_type::egptr() - pBuffer.size(), pBuffer.size());
        offset += pBuffer.size();
    }
    
    /// We can now read safely from the intermediate into our buffer
    std::size_t amount = static_cast<stream_type*>(this)->read(offset, pBuffer.size() - (offset - &pBuffer[0]), sizeof(char_type));
    if (amount != 0)
    {
        /// Update the get pointer and return the character at its location.
        parent_type::setg(&pBuffer[0], offset, offset + amount);
        return traits_type::to_int_type(*parent_type::gptr());
    }
    
    /// End of File - Nothing more to read
    return traits_type::eof();
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::int_type StreamBuffer<stream_type, char_type, traits_type>::overflow(typename StreamBuffer<stream_type, char_type, traits_type>::int_type c)
{
    /// If we're not at the end of the intermediate, we need to add the char to our buffer and immediately flush
    if (!traits_type::eq_int_type(c, traits_type::eof()))
    {
        /// Store the char in our buffer for writing.
        pBuffer.push_back(traits_type::to_char_type(c));
        
        /// If the character is a new-line, we need to sync/flush our buffer immediately
        /// We do this because we have no idea when the end of the stream is.
        /// It's safe to log each line.
        /// We can also remove this and let the destructor handle the flushing (but this is a backup in case)
        if (traits_type::eq_int_type(c, traits_type::to_int_type('\n')))
        {
            return sync();
        }
    }
    
    /// Return Not End of File because we won't ever really "overflow".
    /// There can never be a buffer overflow because we're not buffering data.
    return traits_type::not_eof(c);
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::pos_type StreamBuffer<stream_type, char_type, traits_type>::seekoff(typename StreamBuffer<stream_type, char_type, traits_type>::off_type pos, std::ios_base::seekdir dir, std::ios_base::openmode mode)
{
    /// Handle seeking the stream for different stream operation modes
    if (mode & std::ios_base::in)
    {
        if (mode & std::ios_base::out)
        {
            return pos_type(off_type(-1));
        }
        
        if (static_cast<stream_type*>(this)->seek(pos, dir))
        {
            return pos_type(off_type(-1));
        }
        
        parent_type::setg(parent_type::eback(), parent_type::egptr(), parent_type::egptr());
    }
    else if (mode & std::ios_base::out)
    {
        if (static_cast<stream_type*>(this)->seek(pos, dir))
        {
            return pos_type(off_type(-1));
        }
        parent_type::setp(parent_type::pbase(), parent_type::epptr());
    }
    else
    {
        return pos_type(off_type(-1));
    }
    
    return pos_type(static_cast<stream_type*>(this)->tell());
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::pos_type StreamBuffer<stream_type, char_type, traits_type>::seekpos(typename StreamBuffer<stream_type, char_type, traits_type>::pos_type pos, std::ios_base::openmode mode)
{
    return parent_type::seekoff(off_type(pos), std::ios_base::beg, mode);
}

/// A CRTP stream that writes the data it receives to "callback"/"listener" functions.
/// It's not really buffered at all but there is an internal buffer that stores all the characters
/// entered into the stream waiting to be flushed as soon as we receive `std::endl` or `\\n'
template<typename char_type, typename traits_type = std::char_traits<char_type>>
class BufferedOutputStream : public StreamBuffer<BufferedOutputStream<char_type, traits_type>, char_type, traits_type>, public std::basic_iostream<char_type, traits_type>
{
protected:
    using parent_type = StreamBuffer<BufferedOutputStream<char_type, traits_type>, char_type, traits_type>;
    using pos_type = typename traits_type::pos_type;
    using seekdir = typename std::ios_base::seekdir;
    using openmode = typename std::ios_base::openmode;
    
private:
    std::function<void(std::string)> onWrite;
    std::function<void()> onFlush;
    
public:
    /// Constructs a stream using the underlying `StreamBuffer` implementation
    /// Stores the `onWrite` and `onFlush` callback/listener functions for when the data needs to be logged/handled elsewhere.
    BufferedOutputStream(std::function<void(std::string)> onWrite, std::function<void()> onFlush) : parent_type(), std::basic_iostream<char_type, traits_type>(this), onWrite(onWrite), onFlush(onFlush) {}
    virtual ~BufferedOutputStream() { this->flush(); }
    
    
    virtual std::size_t read(char_type* data, std::size_t amount, std::size_t element_size)
    {
        return 0;
    }
    
    virtual std::size_t write(char_type* data, std::size_t amount, std::size_t element_size)
    {
        onWrite(std::string(data, amount));
        return amount;
    }
    
    virtual bool seek(std::size_t position, seekdir direction)
    {
        return false;
    }
    
    virtual pos_type tell()
    {
        return -1;
    }
    
    int flush()
    {
        onFlush();
        return 0;
    }
};


//Redirects IO to a stringstream
class LogPipe final: std::stringstream {
public:
    LogPipe(std::ostream& fs) : io_buffer(static_cast<std::ostream*>(this)->rdbuf(fs.rdbuf())) { }
    ~LogPipe() override;
    
private:
    std::streambuf* io_buffer;
    
    LogPipe(const LogPipe&) = delete;
    LogPipe& operator=(const LogPipe&) = delete;
};

/// A struct containing all the logging information
struct UnbufferedLoggerData final {
    const void* logger;
    const std::int32_t log_level;
    const std::string file;
    const std::int32_t line;
    const std::string data;
};

/// The Unbuffered Logging class that can log to any given stream or logs via callback/listener functions.
class UnbufferedLogger final {
public:
    UnbufferedLogger(std::ostream& stream) : out_stream(&stream), isHeapAllocated(false) { out_stream->rdbuf()->pubsetbuf(nullptr, 0); }
    
    UnbufferedLogger(std::function<void(std::string)> onWrite, std::function<void()> onFlush) : out_stream(new BufferedOutputStream<char>(onWrite, onFlush)), isHeapAllocated(true) {}
    
    ~UnbufferedLogger();
    
    static void setLoggerCallbacks(std::function<void(UnbufferedLoggerData)> onWrite, std::function<void()> onFlush);
    
    void write(UnbufferedLoggerData data);
    
    void flush();
    
    inline std::ostream& stream() {
        return *out_stream;
    }
    
private:
    std::ostream* out_stream;
    bool isHeapAllocated;
    static std::function<void(UnbufferedLoggerData)> onWrite;
    static std::function<void()> onFlush;
    
    UnbufferedLogger(const UnbufferedLogger&) = delete;
    UnbufferedLogger& operator=(const UnbufferedLogger&) = delete;
};

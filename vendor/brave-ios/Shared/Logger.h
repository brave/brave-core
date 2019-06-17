#include <iostream>
#include <sstream>
#include <streambuf>
#include <cstdint>

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
    std::basic_string<char_type, traits_type, std::allocator<char_type>> pBuffer;
    char_type* curr_ptr;
    
    int sync();
    int_type underflow();
    int_type overflow(int_type c  = traits_type::eof());
    pos_type seekoff(off_type pos, seekdir dir, openmode mode = parent_type::in | parent_type::out);
    pos_type seekpos(pos_type pos, openmode mode = parent_type::in | parent_type::out);
    
public:
    StreamBuffer();
    virtual ~StreamBuffer();
    
    StreamBuffer(const StreamBuffer& other) = delete;
    StreamBuffer& operator = (const StreamBuffer& other) = delete;
};

template<typename stream_type, typename char_type, typename traits_type>
StreamBuffer<stream_type, char_type, traits_type>::StreamBuffer() : pBuffer()
{
    parent_type::setg(nullptr, nullptr, nullptr);
    parent_type::setp(nullptr, nullptr);
}

template<typename stream_type, typename char_type, typename traits_type>
StreamBuffer<stream_type, char_type, traits_type>::~StreamBuffer()
{
    //sync();
}

template<typename stream_type, typename char_type, typename traits_type>
int StreamBuffer<stream_type, char_type, traits_type>::sync()
{
    if (static_cast<stream_type*>(this)->write(&pBuffer[0], pBuffer.size(), sizeof(char_type)) == pBuffer.size())
    {
        pBuffer.clear();
        static_cast<stream_type*>(this)->flush();
        return 0;
    }
    
    return traits_type::eof();
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::int_type StreamBuffer<stream_type, char_type, traits_type>::underflow()
{
    if (parent_type::gptr() < parent_type::egptr())
    {
        return traits_type::to_int_type(*parent_type::gptr());
    }
    else
    {
        std::size_t pOffset = std::abs(parent_type::pptr() - parent_type::pbase());
        std::size_t gOffset = std::abs(parent_type::gptr() - parent_type::eback());
        
        pBuffer.push_back(char_type());
        
        parent_type::setg(&pBuffer[0], &pBuffer[0] + gOffset, &pBuffer[0] + pBuffer.size());
        parent_type::setp(&pBuffer[0], &pBuffer[0] + pBuffer.size());
        parent_type::pbump(static_cast<int>(pOffset));
    }
    
    char_type* offset = &pBuffer[0];
    if (parent_type::eback() == &pBuffer[0])
    {
        std::memmove(&pBuffer[0], parent_type::egptr() - pBuffer.size(), pBuffer.size());
        offset += pBuffer.size();
    }
    
    std::size_t amount = static_cast<stream_type*>(this)->read(offset, pBuffer.size() - (offset - &pBuffer[0]), sizeof(char_type));
    if (amount != 0)
    {
        parent_type::setg(&pBuffer[0], offset, offset + amount);
        return traits_type::to_int_type(*parent_type::gptr());
    }
    
    return traits_type::eof();
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::int_type StreamBuffer<stream_type, char_type, traits_type>::overflow(typename StreamBuffer<stream_type, char_type, traits_type>::int_type c)
{
    if (!traits_type::eq_int_type(c, traits_type::eof()))
    {
        pBuffer.push_back(traits_type::to_char_type(c));
        if (traits_type::eq_int_type(c, traits_type::to_int_type('\n')))
        {
            return sync();
        }
    }
    return traits_type::not_eof(c);
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::pos_type StreamBuffer<stream_type, char_type, traits_type>::seekoff(typename StreamBuffer<stream_type, char_type, traits_type>::off_type pos, std::ios_base::seekdir dir, std::ios_base::openmode mode)
{
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

struct UnbufferedLoggerData final {
    const void* logger;
    const std::int32_t log_level;
    const std::string file;
    const std::int32_t line;
    const std::string data;
};

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

/**
 * This file is part of png++ the C++ wrapper for libpng.  Png++ is free
 * software; the exact copying conditions are as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef PNGPP_WRITER_HPP_INCLUDED
#define PNGPP_WRITER_HPP_INCLUDED

#include <cassert>
#include <iostream>
#include "io_base.hpp"

namespace png
{

    class writer
        : public io_base
    {
    public:
        explicit writer(std::ostream& stream)
            : io_base(png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                              static_cast< io_base* >(this),
                                              raise_error,
                                              0))
        {
            png_set_write_fn(m_png, & stream, write_data, flush_data);
        }

        ~writer(void)
        {
            m_info.destroy();
            m_end_info.destroy();
            png_destroy_write_struct(& m_png, 0);
        }

        void write_png(void) const
        {
            if (setjmp(png_jmpbuf(m_png)))
            {
                throw error(m_error);
            }
            png_write_png(m_png,
                          m_info.get_png_struct(),
                          /* transforms = */ 0,
                          /* params = */ 0);
        }

        void write_info(void) const
        {
            if (setjmp(png_jmpbuf(m_png)))
            {
                throw error(m_error);
            }
            m_info.write();
        }

        template< typename pixbuf >
        void write_image(pixbuf& buf) const
        {
            if (setjmp(png_jmpbuf(m_png)))
            {
                throw error(m_error);
            }
#ifdef PNG_WRITE_INTERLACING_SUPPORTED
            int pass = set_interlace_handling();
#else
            int pass = 1;
#endif
            while (pass-- > 0)
            {
                for (size_t row = 0; row < buf.get_height(); ++row)
                {
                    byte* pixels
                        = reinterpret_cast< byte* >(& buf.get_row(row).at(0));
                    png_write_row(m_png, pixels);
                }
            }
        }

        void write_end_info(void) const
        {
            if (setjmp(png_jmpbuf(m_png)))
            {
                throw error(m_error);
            }
            m_end_info.write();
        }

    private:
        static void write_data(png_struct* png, byte* data, size_t length)
        {
            io_base* io = static_cast< io_base* >(png_get_error_ptr(png));
            writer* wr = static_cast< writer* >(io);
            wr->reset_error();
            std::ostream* stream
                = reinterpret_cast< std::ostream* >(png_get_io_ptr(png));
            try
            {
                stream->write(reinterpret_cast< char* >(data), length);
                if (!stream->good())
                {
                    wr->set_error("std::istream::write() failed");
                }
            }
            catch (std::exception const& error)
            {
                wr->set_error(error.what());
            }
            catch (...)
            {
                assert(!"caught something wrong");
            }
            if (wr->is_error())
            {
                wr->raise_error();
            }
        }

        static void flush_data(png_struct* png)
        {
            io_base* io = static_cast< io_base* >(png_get_error_ptr(png));
            writer* wr = static_cast< writer* >(io);
            wr->reset_error();
            std::ostream* stream
                = reinterpret_cast< std::ostream* >(png_get_io_ptr(png));
            try
            {
                stream->flush();
                if (!stream->good())
                {
                    wr->set_error("std::istream::flush() failed");
                }
            }
            catch (std::exception const& error)
            {
                wr->set_error(error.what());
            }
            catch (...)
            {
                assert(!"caught something wrong");
            }
            if (wr->is_error())
            {
                wr->raise_error();
            }
        }
    };

} // namespace png

#endif // PNGPP_WRITER_HPP_INCLUDED

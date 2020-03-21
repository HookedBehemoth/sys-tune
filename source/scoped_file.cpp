/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "scoped_file.hpp"

namespace ams {

    namespace {

        /* Convenience definitions. */
        constexpr size_t MaximumLineLength = 0x20;

        os::Mutex g_format_lock;
        char g_format_buffer[2 * os::MemoryPageSize];

    }

    void ScopedFile::WriteString(const char *str) {
        this->Write(str, std::strlen(str));
    }

    void ScopedFile::WriteFormat(const char *fmt, ...) {
        /* Acquire exclusive access to the format buffer. */
        std::scoped_lock lk(g_format_lock);

        /* Format to the buffer. */
        {
            std::va_list vl;
            va_start(vl, fmt);
            std::vsnprintf(g_format_buffer, sizeof(g_format_buffer), fmt, vl);
            va_end(vl);
        }

        /* Write data. */
        this->WriteString(g_format_buffer);
    }

    void ScopedFile::Write(const void *data, size_t size) {
        /* If we're not open, we can't write. */
        if (!this->IsOpen()) {
            return;
        }

        /* Advance, if we write successfully. */
        if (R_SUCCEEDED(fs::WriteFile(this->file, this->offset, data, size, fs::WriteOption::Flush))) {
            this->offset += size;
        }
    }
}

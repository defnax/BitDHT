/*
 * tests/bencode_bounds_test.c
 *
 * BitDHT.
 *
 * Copyright 2026 by defnax.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 *
 */

#include "bitdht/bencode.h"
#include <assert.h>
#include <limits.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

/* Place the exact packet end against an inaccessible page, without a NUL. */
static be_node *decode(const char *text, size_t len)
{
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    size_t page = info.dwPageSize;
    char *buffer = (char *)VirtualAlloc(NULL, page * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    DWORD old;
    assert(buffer && VirtualProtect(buffer + page, page, PAGE_NOACCESS, &old));
#else
    size_t page = sysconf(_SC_PAGESIZE);
    char *buffer = (char *)mmap(NULL, page * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(buffer != MAP_FAILED);
    assert(mprotect(buffer + page, page, PROT_NONE) == 0);
#endif
    assert(len <= page);
    memcpy(buffer + page - len, text, len);
    be_node *node = be_decoden(buffer + page - len, len);
#ifdef _WIN32
    VirtualFree(buffer, 0, MEM_RELEASE);
#else
    munmap(buffer, page * 2);
#endif
    return node;
}

int main(void)
{
    const char *bad[] = {"", "i", "i123", "i-", "ie", "i+1e", "i 1e",
        "i9223372036854775808e", "i-9223372036854775809e", "123", "1:",
        "999999999999999999999999:", "l", "li1e", "d", "d1:a", "d1:ai1e", "d1:ae"};
    for (size_t i = 0; i < sizeof(bad)/sizeof(*bad); ++i)
        assert(decode(bad[i], strlen(bad[i])) == NULL);
    const char *good[] = {"i0e", "i-1e", "i9223372036854775807e", "i-9223372036854775808e",
        "0:", "3:abc", "le", "de", "li1ee", "d1:ai1ee", "d1:ald1:bi2eeee"};
    for (size_t i = 0; i < sizeof(good)/sizeof(*good); ++i) {
        size_t len = strlen(good[i]);
        be_node *n = decode(good[i], len);
        assert(n);
        if (i == 2) assert(n->val.i == LLONG_MAX);
        if (i == 3) assert(n->val.i == LLONG_MIN);
        be_free(n);
        for (size_t j = 0; j < len; ++j) assert(decode(good[i], j) == NULL);
    }
    assert(be_decoden(NULL, 1) == NULL);
    assert(be_decoden("i1e", -1) == NULL);
    return 0;
}

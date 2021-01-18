/*
 * This file is part of the BitHD project, https://bithd.com/
 *
 * Copyright (C) 2020 Bitpie
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ERRMSG_H__
#define __ERRMSG_H__

#include <stdio.h>

typedef char MSG[128];
inline int errmsg(MSG msg_buf, const int error_code, const char* err_msg) {
    snprintf(msg_buf, sizeof(MSG), "[%s:%d] %s", __FILE__, __LINE__, err_msg);
    return error_code;
}

#endif // __ERRMSG_H__
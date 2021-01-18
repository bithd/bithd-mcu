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

#ifndef __TRON_ERR_H__
#define __TRON_ERR_H__

enum TRON_ERROR_CODE {
    E_TRON_DecodeTriggerSmartContract   = 0x30000001,
    E_TRON_EncodeTronAddress            = 0x30000002,
    E_TRON_InvalidMethodSignature       = 0x30000003,
    E_TRON_InvalidContractDataSize      = 0x30000004,
    E_TRON_InvalidAddress               = 0x30000005,
    E_TRON_InvalidCallData              = 0x30000006,
    E_TRON_DecodeTransferContract       = 0x30000007,
    E_TRON_UnsupportedToken             = 0x30000008,
};

#endif // __TRON_ERR_H__
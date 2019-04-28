#include "eos_reader.h"

bool check_available(EosReaderCTX* reader_ctx, int num) 
{
    return reader_ctx->_length - reader_ctx->_index > num;
}

void reader_init(EosReaderCTX* reader_ctx, uint8_t * buf, int len) 
{
    reader_ctx->_buf = buf;
    reader_ctx->_index = 0;
    reader_ctx->_length = len;
}

bool reader_get(EosReaderCTX* reader_ctx, uint8_t * b) 
{
    if(check_available(reader_ctx, 1)) {
        *b = reader_ctx->_buf[reader_ctx->_index++];
        return true;
    }
    return false;
}


bool reader_get_short(EosReaderCTX* reader_ctx, uint16_t * s)
{
    if(check_available(reader_ctx, 2)){
        *s = (((reader_ctx->_buf[reader_ctx->_index++] & 0xFF)) 
            | ((reader_ctx->_buf[reader_ctx->_index++] & 0xFF) << 8)) 
            & 0xFFFF;
        return true;
    }
    return false;
}

bool reader_get_int(EosReaderCTX* reader_ctx, uint32_t *i)
{
    if(check_available(reader_ctx, 4)){
        *i = (((reader_ctx->_buf[reader_ctx->_index++] & 0xFF)) 
            | ((reader_ctx->_buf[reader_ctx->_index++] & 0xFF) << 8) 
            | ((reader_ctx->_buf[reader_ctx->_index++] & 0xFF) << 16)
            | ((reader_ctx->_buf[reader_ctx->_index++] & 0xFF) << 24))
            & 0xFFFFFFFF;
        return true;
    }
    return false;
}

bool reader_get_long(EosReaderCTX* reader_ctx, uint64_t *l)
{
    if(check_available(reader_ctx, 8)){
        *l = (((uint64_t)(reader_ctx->_buf[reader_ctx->_index++] & 0xFFL)) 
            | (((uint64_t)reader_ctx->_buf[reader_ctx->_index++] & 0xFFL) << 8) 
            | (((uint64_t)reader_ctx->_buf[reader_ctx->_index++] & 0xFFL) << 16)
            | (((uint64_t)reader_ctx->_buf[reader_ctx->_index++] & 0xFFL) << 24) 
            | (((uint64_t)reader_ctx->_buf[reader_ctx->_index++] & 0xFFL) << 32) 
            | (((uint64_t)reader_ctx->_buf[reader_ctx->_index++] & 0xFFL) << 40)
            | (((uint64_t)reader_ctx->_buf[reader_ctx->_index++] & 0xFFL) << 48) 
            | (((uint64_t)reader_ctx->_buf[reader_ctx->_index++] & 0xFFL) << 56)) 
            & 0xFFFFFFFFFFFFFFFF;
        return true;
    }
    return false;
}

bool reader_get_bytes(EosReaderCTX* reader_ctx, uint8_t *bytes, size_t len) 
{
    if(check_available(reader_ctx, len)) {
        memcpy(bytes, reader_ctx->_buf + reader_ctx->_index, len);
        reader_ctx->_index += len;
        return true;
    }
    return false;
}

bool reader_get_variable_uint(EosReaderCTX* reader_ctx, uint64_t *val)
{
    int i = 0;
    uint64_t v = 0;
    uint8_t b, by = 0;
    do {
        if (reader_get(reader_ctx, &b)) {
            v |= ( b & 0x7F) << by;
            by +=7;
            i ++;
        } else {
            break;
        }
    } while ( (b & 0x80) != 0 );
    *val = v;
    return i > 0;
}
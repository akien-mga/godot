/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_compress.h"
#include "internal_decompress.h"

#include "internal_coding.h"
#include "internal_xdr.h"

#include <string.h>
#include "openexr_compression.h"

/**************************************/

static exr_result_t
undo_pxr24_impl (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size,
    void*                  scratch_data,
    uint64_t               scratch_size)
{
    size_t         outSize;
    exr_result_t   rstat;
    uint8_t*       out    = uncompressed_data;
    uint64_t       nOut   = 0;
    uint64_t       nDec   = 0;
    const uint8_t* lastIn = scratch_data;

    if (scratch_size < uncompressed_size) return EXR_ERR_INVALID_ARGUMENT;

    rstat = exr_uncompress_buffer (
        decode->context,
        compressed_data,
        comp_buf_size,
        scratch_data,
        scratch_size,
        &outSize);

    if (rstat != EXR_ERR_SUCCESS) return rstat;

    for (int y = 0; y < decode->chunk.height; ++y)
    {
        int cury = y + decode->chunk.start_y;

        for (int c = 0; c < decode->channel_count; ++c)
        {
            const exr_coding_channel_info_t* curc = decode->channels + c;
            int                              w    = curc->width;
            uint64_t                         nBytes =
                (uint64_t) (w) * (uint64_t) (curc->bytes_per_element);

            if (curc->height == 0 ||
                (curc->y_samples > 1 && (cury % curc->y_samples) != 0))
                continue;

            if (nOut + nBytes > uncompressed_size)
                return EXR_ERR_OUT_OF_MEMORY;

            switch (curc->data_type)
            {
                case EXR_PIXEL_UINT: {
                    const uint8_t* ptr[4];
                    uint32_t       pixel = 0;
                    uint32_t*      dout  = (uint32_t*) (out);

                    ptr[0] = lastIn;
                    lastIn += w;
                    ptr[1] = lastIn;
                    lastIn += w;
                    ptr[2] = lastIn;
                    lastIn += w;
                    ptr[3] = lastIn;
                    lastIn += w;

                    if (nDec + nBytes > outSize)
                        return EXR_ERR_CORRUPT_CHUNK;

                    for (int x = 0; x < w; ++x)
                    {
                        uint32_t diff =
                            (((uint32_t) (*(ptr[0]++)) << 24) |
                             ((uint32_t) (*(ptr[1]++)) << 16) |
                             ((uint32_t) (*(ptr[2]++)) << 8) |
                             ((uint32_t) (*(ptr[3]++))));
                        pixel += diff;
                        unaligned_store32 (dout, pixel);
                        ++dout;
                    }
                    nDec += nBytes;
                    break;
                }
                case EXR_PIXEL_HALF: {
                    const uint8_t* ptr[2];
                    uint32_t       pixel = 0;
                    uint16_t*      dout  = (uint16_t*) (out);

                    ptr[0] = lastIn;
                    lastIn += w;
                    ptr[1] = lastIn;
                    lastIn += w;

                    if (nDec + nBytes > outSize)
                        return EXR_ERR_CORRUPT_CHUNK;

                    for (int x = 0; x < w; ++x)
                    {
                        uint32_t diff =
                            (((uint32_t) (*(ptr[0]++)) << 8) |
                             ((uint32_t) (*(ptr[1]++))));
                        pixel += diff;
                        unaligned_store16 (dout, (uint16_t) pixel);
                        ++dout;
                    }
                    nDec += nBytes;
                    break;
                }
                case EXR_PIXEL_FLOAT: {
                    const uint8_t* ptr[3];
                    uint32_t       pixel = 0;
                    uint32_t*      dout  = (uint32_t*) (out);

                    ptr[0] = lastIn;
                    lastIn += w;
                    ptr[1] = lastIn;
                    lastIn += w;
                    ptr[2] = lastIn;
                    lastIn += w;

                    if (nDec + (uint64_t) w * 3 > outSize)
                        return EXR_ERR_CORRUPT_CHUNK;

                    for (int x = 0; x < w; ++x)
                    {
                        uint32_t diff =
                            (((uint32_t) (*(ptr[0]++)) << 24) |
                             ((uint32_t) (*(ptr[1]++)) << 16) |
                             ((uint32_t) (*(ptr[2]++)) << 8));
                        pixel += diff;
                        unaligned_store32 (dout, pixel);
                        ++dout;
                    }
                    nDec += (uint64_t) w * 3;
                    break;
                }
                default: return EXR_ERR_INVALID_ARGUMENT;
            }
            out += nBytes;
            nOut += nBytes;
        }
    }
    decode->bytes_decompressed = nOut;
    return EXR_ERR_SUCCESS;
}

exr_result_t
internal_exr_undo_pxr24 (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    exr_result_t rv;
    rv = internal_decode_alloc_buffer (
        decode,
        EXR_TRANSCODE_BUFFER_SCRATCH1,
        &(decode->scratch_buffer_1),
        &(decode->scratch_alloc_size_1),
        exr_compress_max_buffer_size (uncompressed_size));
    if (rv != EXR_ERR_SUCCESS) return rv;
    return undo_pxr24_impl (
        decode,
        compressed_data,
        comp_buf_size,
        uncompressed_data,
        uncompressed_size,
        decode->scratch_buffer_1,
        decode->scratch_alloc_size_1);
}

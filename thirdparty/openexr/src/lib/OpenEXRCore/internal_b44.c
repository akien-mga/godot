/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_compress.h"
#include "internal_decompress.h"

#include "internal_coding.h"
#include "internal_xdr.h"

#include <string.h>

/**************************************/

extern uint16_t* exrcore_logTable;
extern void      exrcore_ensure_b44_tables ();

static inline void
convertToLinear (uint16_t s[16])
{
    for (int i = 0; i < 16; ++i)
        s[i] = exrcore_logTable[s[i]];
}

/**************************************/

static inline void
unpack14 (const uint8_t b[14], uint16_t s[16])
{
    uint16_t shift, bias;
    s[0] = ((uint16_t) (b[0] << 8)) | ((uint16_t) b[1]);

    shift = (b[2] >> 2);
    bias  = (uint16_t) (0x20u << shift);

    s[4]  = (uint16_t) ((uint32_t) s[0] +
                       (uint32_t) ((((uint32_t) (b[2] << 4) |
                                     (uint32_t) (b[3] >> 4)) &
                                    0x3fu)
                                   << shift) -
                       bias);
    s[8]  = (uint16_t) ((uint32_t) s[4] +
                       (uint32_t) ((((uint32_t) (b[3] << 2) |
                                     (uint32_t) (b[4] >> 6)) &
                                    0x3fu)
                                   << shift) -
                       bias);
    s[12] = (uint16_t) ((uint32_t) s[8] +
                        (uint32_t) ((uint32_t) (b[4] & 0x3fu) << shift) - bias);

    s[1]  = (uint16_t) ((uint32_t) s[0] +
                       (uint32_t) ((uint32_t) (b[5] >> 2) << shift) - bias);
    s[5]  = (uint16_t) ((uint32_t) s[4] +
                       (uint32_t) ((((uint32_t) (b[5] << 4) |
                                     (uint32_t) (b[6] >> 4)) &
                                    0x3fu)
                                   << shift) -
                       bias);
    s[9]  = (uint16_t) ((uint32_t) s[8] +
                       (uint32_t) ((((uint32_t) (b[6] << 2) |
                                     (uint32_t) (b[7] >> 6)) &
                                    0x3fu)
                                   << shift) -
                       bias);
    s[13] = (uint16_t) ((uint32_t) s[12] +
                        (uint32_t) ((uint32_t) (b[7] & 0x3fu) << shift) - bias);

    s[2]  = (uint16_t) ((uint32_t) s[1] +
                       (uint32_t) ((uint32_t) (b[8] >> 2) << shift) - bias);
    s[6]  = (uint16_t) ((uint32_t) s[5] +
                       (uint32_t) ((((uint32_t) (b[8] << 4) |
                                     (uint32_t) (b[9] >> 4)) &
                                    0x3fu)
                                   << shift) -
                       bias);
    s[10] = (uint16_t) ((uint32_t) s[9] +
                        (uint32_t) ((((uint32_t) (b[9] << 2) |
                                      (uint32_t) (b[10] >> 6)) &
                                     0x3fu)
                                    << shift) -
                        bias);
    s[14] =
        (uint16_t) ((uint32_t) s[13] +
                    (uint32_t) ((uint32_t) (b[10] & 0x3fu) << shift) - bias);

    s[3]  = (uint16_t) ((uint32_t) s[2] +
                       (uint32_t) ((uint32_t) (b[11] >> 2) << shift) - bias);
    s[7]  = (uint16_t) ((uint32_t) s[6] +
                       (uint32_t) ((((uint32_t) (b[11] << 4) |
                                     (uint32_t) (b[12] >> 4)) &
                                    0x3fu)
                                   << shift) -
                       bias);
    s[11] = (uint16_t) ((uint32_t) s[10] +
                        (uint32_t) ((((uint32_t) (b[12] << 2) |
                                      (uint32_t) (b[13] >> 6)) &
                                     0x3fu)
                                    << shift) -
                        bias);
    s[15] =
        (uint16_t) ((uint32_t) s[14] +
                    (uint32_t) ((uint32_t) (b[13] & 0x3fu) << shift) - bias);

    for (int i = 0; i < 16; ++i)
    {
        if (s[i] & 0x8000)
            s[i] &= 0x7fff;
        else
            s[i] = ~s[i];
    }
}

static inline void
unpack3 (const uint8_t b[3], uint16_t s[16])
{
    s[0] = ((uint16_t) (b[0] << 8)) | ((uint16_t) b[1]);

    if (s[0] & 0x8000)
        s[0] &= 0x7fff;
    else
        s[0] = ~s[0];

    for (int i = 1; i < 16; ++i)
        s[i] = s[0];
}

/**************************************/

static exr_result_t
uncompress_b44_impl (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncomp_buf_size)
{
    const uint8_t* in      = compressed_data;
    uint8_t*       out     = uncompressed_data;
    uint8_t*       scratch = decode->scratch_buffer_1;
    uint8_t*       tmp;
    uint16_t *     row0, *row1, *row2, *row3;
    uint64_t       n, nBytes, bpl = 0, bIn = 0;
    int            nx, ny;
    uint16_t       s[16];

    for (int c = 0; c < decode->channel_count; ++c)
    {
        const exr_coding_channel_info_t* curc = decode->channels + c;
        nx                                    = curc->width;
        ny                                    = curc->height;
        nBytes = (uint64_t) (ny) * (uint64_t) (nx) *
                 (uint64_t) (curc->bytes_per_element);

        if (nBytes == 0) continue;

        if (curc->data_type != EXR_PIXEL_HALF)
        {
            if (bIn + nBytes > comp_buf_size) return EXR_ERR_OUT_OF_MEMORY;
            memcpy (scratch, in, nBytes);
            in += nBytes;
            bIn += nBytes;
            scratch += nBytes;
            continue;
        }

        for (int y = 0; y < ny; y += 4)
        {
            /* row offset in elements: use uint64_t so y*nx cannot overflow int */
            uint64_t row_off = (uint64_t) (y) * (uint64_t) (nx);
            row0 = (uint16_t*) scratch + row_off;
            row1 = row0 + (uint64_t) nx;
            row2 = row1 + (uint64_t) nx;
            row3 = row2 + (uint64_t) nx;
            for (int x = 0; x < nx; x += 4)
            {
                if (bIn + 3 > comp_buf_size) return EXR_ERR_OUT_OF_MEMORY;

                /* check if 3-byte encoded flat field */
                if (in[2] >= (13 << 2))
                {
                    unpack3 (in, s);
                    in += 3;
                    bIn += 3;
                }
                else
                {
                    if (bIn + 14 > comp_buf_size) return EXR_ERR_OUT_OF_MEMORY;
                    unpack14 (in, s);
                    in += 14;
                    bIn += 14;
                }

                if (curc->p_linear) convertToLinear (s);

                priv_from_native16 (s, 16);

                n = (x + 3 < nx) ? 4 * sizeof (uint16_t)
                                 : (uint64_t) (nx - x) * sizeof (uint16_t);
                if (y + 3 < ny)
                {
                    memcpy (row0, &s[0], n);
                    memcpy (row1, &s[4], n);
                    memcpy (row2, &s[8], n);
                    memcpy (row3, &s[12], n);
                }
                else
                {
                    memcpy (row0, &s[0], n);
                    if (y + 1 < ny) memcpy (row1, &s[4], n);
                    if (y + 2 < ny) memcpy (row2, &s[8], n);
                }
                row0 += 4;
                row1 += 4;
                row2 += 4;
                row3 += 4;
            }
        }
        scratch += nBytes;
    }

    /* now put it back so each scanline has channel data */
    bIn = 0;
    for (int y = 0; y < decode->chunk.height; ++y)
    {
        int cury = y + decode->chunk.start_y;

        scratch = decode->scratch_buffer_1;
        for (int c = 0; c < decode->channel_count; ++c)
        {
            const exr_coding_channel_info_t* curc = decode->channels + c;

            nx     = curc->width;
            ny     = curc->height;
            bpl    = ((uint64_t) (nx)) * (uint64_t) (curc->bytes_per_element);
            nBytes = ((uint64_t) (ny)) * bpl;

            if (nBytes == 0) continue;

            tmp = scratch;
            if (curc->y_samples > 1)
            {
                if ((cury % curc->y_samples) != 0)
                {
                    scratch += nBytes;
                    continue;
                }
                tmp += ((uint64_t) (y / curc->y_samples)) * bpl;
            }
            else
                tmp += ((uint64_t) y) * bpl;

            if (bIn + bpl > uncomp_buf_size) return EXR_ERR_OUT_OF_MEMORY;

            memcpy (out, tmp, bpl);

            bIn += bpl;
            out += bpl;
            scratch += nBytes;
        }
    }

    decode->bytes_decompressed = uncomp_buf_size;
    return EXR_ERR_SUCCESS;
}

/**************************************/

static uint64_t
compute_scratch_buffer_size (
    exr_decode_pipeline_t* decode, uint64_t uncompressed_size)
{
    const exr_coding_channel_info_t* curc;
    int                              nx, ny;
    uint64_t                         ret  = uncompressed_size;
    uint64_t                         comp = 0;

    for (int c = 0; c < decode->channel_count; ++c)
    {
        curc = decode->channels + c;

        nx = curc->width;
        ny = curc->height;

        if (nx % 4) nx += 4 - nx % 4;
        if (ny % 4) ny += 4 - ny % 4;

        comp += (uint64_t) (ny) * (uint64_t) (nx) *
                (uint64_t) (curc->bytes_per_element);
    }
    if (comp > ret) ret = comp;
    return ret;
}

/**************************************/

exr_result_t
internal_exr_undo_b44 (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    exr_result_t rv;
    uint64_t     scratch_sz = compute_scratch_buffer_size (decode, uncompressed_size);
    if (scratch_sz != (size_t) scratch_sz) return EXR_ERR_OUT_OF_MEMORY;
    rv = internal_decode_alloc_buffer (
        decode,
        EXR_TRANSCODE_BUFFER_SCRATCH1,
        &(decode->scratch_buffer_1),
        &(decode->scratch_alloc_size_1),
        (size_t) scratch_sz);
    if (rv != EXR_ERR_SUCCESS) return rv;

    exrcore_ensure_b44_tables ();

    return uncompress_b44_impl (
        decode,
        compressed_data,
        comp_buf_size,
        uncompressed_data,
        uncompressed_size);
}

exr_result_t
internal_exr_undo_b44a (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    exr_result_t rv;
    uint64_t     scratch_sz = compute_scratch_buffer_size (decode, uncompressed_size);
    if (scratch_sz != (size_t) scratch_sz) return EXR_ERR_OUT_OF_MEMORY;
    rv = internal_decode_alloc_buffer (
        decode,
        EXR_TRANSCODE_BUFFER_SCRATCH1,
        &(decode->scratch_buffer_1),
        &(decode->scratch_alloc_size_1),
        (size_t) scratch_sz);
    if (rv != EXR_ERR_SUCCESS) return rv;

    exrcore_ensure_b44_tables ();

    return uncompress_b44_impl (
        decode,
        compressed_data,
        comp_buf_size,
        uncompressed_data,
        uncompressed_size);
}

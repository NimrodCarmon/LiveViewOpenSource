#ifndef INTERLACEFILTER_H
#define INTERLACEFILTER_H

#include <stdint.h>
#include <QDebug>
#include <cmath>

#include "constants.h"
#include "image_type.h"

class InterlaceFilter
{
public:
    explicit InterlaceFilter(size_t num_rows, size_t num_cols) :
        frSize(num_rows * num_cols), nRows(num_rows), nCols(num_cols),
        nTaps(4), tapWidth(nCols / nTaps)
    {}

    void apply_filter(uint16_t *pic_in)
    {
        size_t row_offset, tap_offset;
        for (size_t r = 0; r < nRows; r++) {
            row_offset = r * nCols;
            for (size_t tap = 0; tap < nTaps; tap++) {
                tap_offset = tap * tapWidth;
                for (size_t x = 0; x < tapWidth; x++) {
                    pic_buffer[row_offset + tap_offset + x] = pic_in[row_offset + nTaps * x + tap];
                }
            }
        }
        memcpy(pic_in, pic_buffer, frSize * sizeof(uint16_t));
    }

private:
    uint16_t pic_buffer[MAX_SIZE];
    size_t frSize;
    size_t nRows;
    size_t nCols;
    size_t nTaps;
    size_t tapWidth;
};

#endif // INTERLACEFILTER_H

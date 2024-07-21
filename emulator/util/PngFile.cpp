/*  This file is part of UKNCBTL.
    UKNCBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    UKNCBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
UKNCBTL. If not, see <http://www.gnu.org/licenses/>. */

// PngFile.cpp

#include "stdafx.h"
#include "PngFile.h"

//////////////////////////////////////////////////////////////////////


bool PngFile_SaveImage(
    const char* pstrFileName, void* pImageData,
    int iWidth, int iHeight)
{
    const int ciBitDepth = 8;
    const int ciChannels = 4;

    png_byte* pDiData = (png_byte*)pImageData;

    if (pstrFileName == nullptr)
        return false;

    // open the PNG output file
    FILE* pfFile = fopen(pstrFileName, "wb");
    if (!pfFile)
        return false;

    // prepare the standard PNG structures
    png_structp png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING, nullptr, (png_error_ptr)nullptr, (png_error_ptr)nullptr);
    if (!png_ptr)
    {
        fclose(pfFile);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        fclose(pfFile);
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        return false;
    }

    // initialize the png structure
    png_init_io(png_ptr, pfFile);

    // we're going to write a very simple 4x8-bit RGBA image
    png_set_IHDR(
        png_ptr, info_ptr, iWidth, iHeight, ciBitDepth,
        PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);

    // write the file header information
    png_write_info(png_ptr, info_ptr);

    // swap the BGR pixels in the DiData structure to RGB
    png_set_bgr(png_ptr);

    // row_bytes is the width x number of channels
    png_uint_32 ulRowBytes = iWidth * ciChannels;

    // we can allocate memory for an array of row-pointers
    png_bytepp ppbRowPointers = (png_bytepp)malloc(iHeight * sizeof(png_bytep));
    if (ppbRowPointers == nullptr)
    {
        //    Throw "Out of memory";
        goto error_cleanup;
    }

    // set the individual row-pointers to point at the correct offsets
    for (int i = 0; i < iHeight; i++)
        ppbRowPointers[i] = pDiData + i * (((ulRowBytes + 3) >> 2) << 2);

    // write out the entire image data in one call
    png_write_image(png_ptr, ppbRowPointers);

    // write the additional chunks to the PNG file (not really needed)
    png_write_end(png_ptr, info_ptr);

    // and we're done
    free(ppbRowPointers);
    ppbRowPointers = NULL;

    // clean up after the write, and free any memory allocated
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

    // yepp, done
    fclose(pfFile);

    return true;

error_cleanup:
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

    if (ppbRowPointers)
        free(ppbRowPointers);

    fclose(pfFile);

    return false;
}


//////////////////////////////////////////////////////////////////////


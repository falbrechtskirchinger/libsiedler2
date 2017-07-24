// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "libSiedler2Defines.h" // IWYU pragma: keep
#include "ArchivItem_Bitmap.h"
#include "ArchivItem_Palette.h"
#include "ArchivInfo.h"
#include "prototypen.h"
#include "libsiedler2.h"
#include "IAllocator.h"
#include "libendian/src/EndianIStreamAdapter.h"
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/filesystem/path.hpp> // For UTF8 support
#include <iostream>
#include <stdexcept>

/**
 *  lädt eine LBM-File in ein ArchivInfo.
 *
 *  @param[in]  file    Dateiname der LBM-File
 *  @param[out] items   ArchivInfo-Struktur, welche gefüllt wird
 *
 *  @return Null bei Erfolg, ein Wert ungleich Null bei Fehler
 */
int libsiedler2::loader::LoadLBM(const std::string& file, ArchivInfo& items)
{
    if(file.empty())
        return 1;

    // Datei zum lesen öffnen
    boost::iostreams::mapped_file_source mmapFile;
    try{
        mmapFile.open(bfs::path(file));
    }catch(std::exception& e){
        std::cerr << "Could not open '" << file << "': " << e.what() << std::endl;
        return 2;
    }
    typedef boost::iostreams::stream<boost::iostreams::mapped_file_source> MMStream;
    MMStream mmapStream(mmapFile);
    libendian::EndianIStreamAdapter<true, MMStream& > lbm(mmapStream);

    // hat das geklappt?
    if(!lbm)
        return 2;

    char header[4], pbm[4];
    // Header einlesen
    lbm >> header;

    // ist es eine LBM-File? (Header "FORM")
    if(strncmp(header, "FORM", 4) != 0)
        return 4;

    // Länge einlesen
    uint32_t length;
    lbm >> length;

    // Typ einlesen
    lbm >> pbm;

    // ist es eine LBM-File? (Typ "PBM ")
    if(strncmp(pbm, "PBM ", 4) != 0)
        return 7;

    boost::interprocess::unique_ptr< baseArchivItem_Bitmap, Deleter<baseArchivItem_Bitmap> > bitmap(dynamic_cast<baseArchivItem_Bitmap*>(getAllocator().create(BOBTYPE_BITMAP_RAW)));
    bitmap->setFormat(FORMAT_PALETTED);

    uint16_t width, height;
    uint16_t compression;
    uint32_t chunk;
    // Chunks einlesen
    while(lbm.read(chunk))
    {
        switch(chunk)
        {
            case 0x424D4844: // "BHMD"
            {

                // Länge einlesen
                lbm >> length;

                // Bei ungerader Zahl aufrunden
                if(length & 1)
                    ++length;

                lbm >> width >> height;

                bitmap->setWidth(width);
                bitmap->setHeight(height);

                // Unbekannte Daten ( 4 Byte ) berspringen
                lbm.ignore(4);

                // Farbtiefe einlesen
                uint16_t depth;
                lbm >> depth;

                // Nur 256 Farben und nicht mehr!
                if(depth != 256 * 8)
                    return 13;

                // Kompressionflag lesen
                lbm >> compression;

                // Keine bekannte Kompressionsart?
                if(compression != 0 && compression != 256)
                    return 15;

                length -= 12;

                // Rest überspringen
                lbm.ignore(length);
            } break;
            case 0x434D4150: // "CMAP"
            {
                // Länge einlesen
                lbm >> length;

                // Bei ungerader Zahl aufrunden
                if(length & 1)
                    ++length;

                // Ist Länge wirklich so groß wie Farbtabelle?
                if(length != 256 * 3)
                    return 17;

                // Daten von Item auswerten
                ArchivItem_Palette* palette = dynamic_cast<ArchivItem_Palette*>(getAllocator().create(BOBTYPE_PALETTE));
                bitmap->setPalette(palette);
                if(palette->load(lbm.getStream(), false) != 0)
                    return 18;
            } break;
            case 0x424F4459: // "BODY"
            {
                // Länge einlesen
                lbm >> length;

                // Bei ungerader Zahl aufrunden
                if(length & 1)
                    ++length;

                // haben wir eine Palette erhalten?
                if(bitmap->getPalette() == NULL)
                    return 20;

                bitmap->tex_alloc();

                switch(compression)
                {
                    case 0: // unkomprimiert
                    {
                        if(length != static_cast<uint32_t>(width * height))
                            return 222;
                        for(int y = 0; y<height; ++y)
                            for(int x = 0; x<width; ++x)
                            {
                                uint8_t color;
                                lbm >> color;
                                bitmap->tex_setPixel(x, y, color);
                            }
                    } break;
                    case 256: // komprimiert (RLE?)
                    {
                        // Welcher Pixel ist dran?
                        uint16_t x = 0, y = 0;

                        // Solange einlesen, bis Block zuende bzw. Datei zuende ist
                        while(length > 0 && !lbm.eof())
                        {
                            // Typ lesen
                            signed char ctype;
                            lbm >> ctype;
                            --length;
                            if(length == 0)
                                continue;

                            if(ctype > 0) // unkomprimierte Pixel
                            {
                                int16_t count = 1 + static_cast<int16_t>(ctype);

                                for(int16_t i = 0; i < count; ++i)
                                {
                                    // Farbe auslesen
                                    uint8_t color;
                                    lbm >> color;
                                    --length;

                                    bitmap->tex_setPixel(x++, y, color);
                                    if(x >= width)
                                    {
                                        ++y;
                                        x = 0;
                                    }
                                }
                            }
                            else // komprimierte Pixel
                            {
                                int16_t count = 1 - static_cast<int16_t>(ctype);

                                // Farbe auslesen
                                uint8_t color;
                                lbm >> color;
                                --length;

                                for(uint16_t i = 0; i < count; ++i)
                                {
                                    bitmap->tex_setPixel(x++, y, color);
                                    if(x >= width)
                                    {
                                        ++y;
                                        x = 0;
                                    }
                                }
                            }
                        }
                    } break;
                }
                items.set(0, bitmap.release());
            } break;
            default:
            {
                // Länge einlesen
                lbm >> length;

                // Bei ungerader Zahl aufrunden
                if(length & 1)
                    ++length;

                // Rest überspringen
                lbm.ignore(length);
            } break;
        }
    }

    if(items.empty() || !lbm.eof())
        return 25;

    return 0;
}

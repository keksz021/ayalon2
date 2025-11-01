#include "StdAfx.h"
#include "PythonWindow.h"
#include "../EterPack/EterPackManager.h"
#include "../EterBase/MappedFile.h"
#include "../EterLib/GrpImageInstance.h"
#include "../EterLib/GrpImage.h"
#include "../EterLib/GrpDevice.h"
#include "../EterLib/StateManager.h"
// #include "StateManager.h" // csak akkor kell, ha OnRender-ben CLAMP-ot állítasz
#include <wingdi.h>

extern "C" {
#include <gif_lib.h>
}

#include <vector>
#include <algorithm>
#include <cstring>
#define _NO_WMIMACROS_
#define _INC_INSTANCE

extern CGraphicDevice* gs_pGraphicDevice;

namespace UI
{

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    uint32_t CGifImageBox::Type()
    {
        static uint32_t s_dwType = GetCRC32("CGifImageBox", strlen("CGifImageBox"));
        return (s_dwType);
    }

    BOOL CGifImageBox::OnIsType(uint32_t dwType)
    {
        if (CGifImageBox::Type() == dwType)
            return TRUE;

        return FALSE;
    }

    CGifImageBox::CGifImageBox(PyObject* ppyObject)
        : CAniImageBox(ppyObject)
    {
    }
    CGifImageBox::~CGifImageBox()
    {
        UnloadGif();
    }

    // Segéd: transparency index kinyerése a GCE-ből
    static int GetTransparentIndex(const SavedImage* frame) {
        if (!frame) return -1;
        for (int i = 0; i < frame->ExtensionBlockCount; ++i) {
            const ExtensionBlock& eb = frame->ExtensionBlocks[i];
            if (eb.Function == GRAPHICS_EXT_FUNC_CODE && eb.ByteCount >= 4) {
                const uint8_t* b = reinterpret_cast<const uint8_t*>(eb.Bytes);
                if (b[0] & 0x01) // Transparency Flag
                    return b[3]; // Transparent Color Index
            }
        }
        return -1;
    }

    // 32 bites BGRA BMP készítése EGY frame-ből, a teljes logikai vászonra helyezve
    static bool RawFrameToBmpImage_BGRA(
        SavedImage* currentFrame,
        GifFileType* gifFile,
        std::vector<uint8_t>& outData
    ) {
        if (!currentFrame || !gifFile || !gifFile->SColorMap)
            return false;

        const GifImageDesc& desc = currentFrame->ImageDesc;
        const ColorMapObject* cmap = desc.ColorMap ? desc.ColorMap : gifFile->SColorMap;
        if (!cmap || !cmap->Colors) return false;

        // Teljes vászon (LOGICAL SCREEN)
        const int canvasW = gifFile->SWidth;
        const int canvasH = gifFile->SHeight;

        // A frame saját mérete és eltolása
        const int W = desc.Width;
        const int H = desc.Height;
        const int offX = desc.Left;
        const int offY = desc.Top;

        // Kimeneti 32bpp BGRA BMP (bottom-up)
        const uint32_t bytesPerPixel = 4;
        const uint32_t rowStride = canvasW * bytesPerPixel;
        const uint32_t pixelBytes = rowStride * canvasH;

        BITMAPFILEHEADER bf = {};
        bf.bfType = 0x4D42; // "BM"
        bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bf.bfSize = bf.bfOffBits + pixelBytes;

        BITMAPINFOHEADER bi = {};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = canvasW;
        bi.biHeight = canvasH;             // bottom-up
        bi.biPlanes = 1;
        bi.biBitCount = 32;                 // 32 bpp
        bi.biCompression = BI_RGB;
        bi.biSizeImage = pixelBytes;

        outData.resize(bf.bfSize);
        std::memcpy(outData.data(), &bf, sizeof(bf));
        std::memcpy(outData.data() + sizeof(bf), &bi, sizeof(bi));

        // BGRA vászon nullázása (A=0 -> teljesen átlátszó)
        uint8_t* dst = outData.data() + bf.bfOffBits;
        std::memset(dst, 0x00, pixelBytes);

        // Interlace kezelés
        const uint8_t* src = reinterpret_cast<const uint8_t*>(currentFrame->RasterBits);
        const bool interlaced = !!desc.Interlace;
        const int transparent = GetTransparentIndex(currentFrame);

        auto put = [&](int x, int y, uint8_t idx) {
            if (x < 0 || y < 0 || x >= canvasW || y >= canvasH) return;
            if (transparent >= 0 && idx == (uint8_t)transparent) return; // hagyd átlátszón

            const GifColorType& c = cmap->Colors[idx];
            // bottom-up BMP sorindex
            const int by = (canvasH - 1 - y);
            uint8_t* p = dst + by * rowStride + x * bytesPerPixel;
            p[0] = c.Blue;
            p[1] = c.Green;
            p[2] = c.Red;
            p[3] = 255; // alpha
            };

        if (!interlaced) {
            // sorfolytonos
            for (int y = 0; y < H; ++y) {
                const uint8_t* row = src + y * W;
                for (int x = 0; x < W; ++x)
                    put(offX + x, offY + y, row[x]);
            }
        }
        else {
            // GIF interlace 4 pass
            const int starts[4] = { 0, 4, 2, 1 };
            const int steps[4] = { 8, 8, 4, 2 };
            int offset = 0;
            for (int p = 0; p < 4; ++p) {
                for (int y = starts[p]; y < H; y += steps[p]) {
                    const uint8_t* row = src + offset;
                    for (int x = 0; x < W; ++x)
                        put(offX + x, offY + y, row[x]);
                    offset += W;
                }
            }
        }

        return true;
    }

    // Memória-olvasó giflibhez
    struct GifMemReader {
        const unsigned char* data = nullptr;
        size_t size = 0;
        size_t pos = 0;
    };
    static int GifReadFunc(GifFileType* file, GifByteType* buf, int sz) {
        GifMemReader* r = reinterpret_cast<GifMemReader*>(file->UserData);
        if (!r || r->pos >= r->size) return 0;
        int n = int(std::min<size_t>(sz, r->size - r->pos));
        memcpy(buf, r->data + r->pos, n);
        r->pos += n;
        return n;
    }

    void CGifImageBox::OnRender()
    {
        if (m_ImageVector.empty())
            return;

        DWORD now = ELTimer_GetMSec();
        DWORD delay = (m_iCurFrame < m_FrameDelayMS.size()) ? m_FrameDelayMS[m_iCurFrame] : 100;
        if (now - m_dwLastTick >= delay)
        {
            m_dwLastTick = now;
            m_iCurFrame = (m_iCurFrame + 1) % m_ImageVector.size();
        }

        CGraphicExpandedImageInstance* inst = m_ImageVector[m_iCurFrame];
        if (!inst)
            return;

        inst->SetPosition((float)m_rect.left, (float)m_rect.top);
        inst->Render();
    }


    bool CGifImageBox::LoadGif(const char* c_szFileName)
    {
        if (!c_szFileName || !*c_szFileName)
            return false;

        UnloadGif(); // előző frame-ek törlése

        CMappedFile file;
        LPCVOID pData = nullptr;

        // PACK-ból beolvasás (a te 3 paraméteres Get-ed szerint)
        if (!CEterPackManager::Instance().Get(file, c_szFileName, &pData)) {
            TraceError("GIF not found in pack: %s", c_szFileName);
            return false;
        }

        if (!pData || !file.Size()) {
            TraceError("GIF mapped but empty: %s", c_szFileName);
            return false;
        }

        GifMemReader reader;
        reader.data = reinterpret_cast<const unsigned char*>(pData);
        reader.size = file.Size();
        reader.pos = 0;

        int error = 0;
        GifFileType* gif = DGifOpen(&reader, GifReadFunc, &error);
        if (!gif) {
            const char* err = GifErrorString(error);
            TraceError("DGifOpen failed for %s: %s", c_szFileName, err ? err : "unknown");
            return false;
        }

        if (DGifSlurp(gif) == GIF_ERROR) {
#if GIFLIB_MAJOR >= 5
            DGifCloseFile(gif, &error);
#else
            DGifCloseFile(gif);
#endif
            TraceError("Failed to read GIF file: %s", c_szFileName);
            return false;
        }

        const int CW = gif->SWidth, CH = gif->SHeight;
        const uint32_t BPP = 4, STRIDE = CW * BPP, PIXBYTES = STRIDE * CH;

        // aktuális vászon + „previous” mentés (disposal=3)
        std::vector<uint8_t> canvas(PIXBYTES, 0x00);
        std::vector<uint8_t> prevCanvas;

        // segéd a GCE kiolvasásához
        auto readGCE = [](const SavedImage* f, int& transparent, uint32_t& delay_ms, int& disposal) {
            transparent = -1; delay_ms = 100; disposal = 0;
            if (!f) return;
            for (int i = 0;i < f->ExtensionBlockCount;++i) {
                const ExtensionBlock& eb = f->ExtensionBlocks[i];
                if (eb.Function == GRAPHICS_EXT_FUNC_CODE && eb.ByteCount >= 4) {
                    const uint8_t* b = (const uint8_t*)eb.Bytes;
                    if (b[0] & 0x01) transparent = b[3];
                    uint32_t h = uint32_t(b[1]) | (uint32_t(b[2]) << 8);
                    if (h) delay_ms = std::max<uint32_t>(10, h * 10);
                    disposal = (b[0] >> 2) & 0x7;
                    break;
            }
            }
    };

        m_ImageVector.clear();
        m_FrameDelayMS.clear();
        m_iCurFrame = 0;
        m_dwLastTick = GetTickCount();

        for (int i = 0; i < gif->ImageCount; ++i)
        {
            SavedImage* img = &gif->SavedImages[i];
            const GifImageDesc& d = img->ImageDesc;
            const ColorMapObject* cmap = d.ColorMap ? d.ColorMap : gif->SColorMap;
            if (!cmap || !cmap->Colors) continue;

            const uint8_t* src = (const uint8_t*)img->RasterBits;

            int transparent, disposal;
            uint32_t delay_ms;
            readGCE(img, transparent, delay_ms, disposal);

            auto put = [&](int x, int y, uint8_t idx) {
                if (x < 0 || y < 0 || x >= CW || y >= CH) return;
                if (transparent >= 0 && idx == (uint8_t)transparent) return;
                const GifColorType& c = cmap->Colors[idx];
                int by = CH - 1 - y;
                uint8_t* p = &canvas[by * STRIDE + x * BPP];
                p[0] = c.Blue; p[1] = c.Green; p[2] = c.Red; p[3] = 255;
                };

            // rászínezés az AKTUÁLIS canvasra
            if (!d.Interlace) {
                for (int y = 0; y < d.Height; ++y) {
                    const uint8_t* row = src + y * d.Width;
                    for (int x = 0; x < d.Width; ++x) put(d.Left + x, d.Top + y, row[x]);
                }
            }
            else {
                static const int starts[4] = { 0,4,2,1 }, steps[4] = { 8,8,4,2 };
                int off = 0;
                for (int p = 0;p < 4;++p)
                    for (int y = starts[p]; y < d.Height; y += steps[p]) {
                        const uint8_t* row = src + off;
                        for (int x = 0; x < d.Width; ++x) put(d.Left + x, d.Top + y, row[x]);
                        off += d.Width;
                    }
            }

            // BMP felépítése a TELJES canvasból
            BITMAPFILEHEADER bf = {};
            bf.bfType = 0x4D42;
            bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            bf.bfSize = bf.bfOffBits + PIXBYTES;

            BITMAPINFOHEADER bi = {};
            bi.biSize = sizeof(BITMAPINFOHEADER);
            bi.biWidth = CW; bi.biHeight = CH;
            bi.biPlanes = 1; bi.biBitCount = 32;
            bi.biCompression = BI_RGB; bi.biSizeImage = PIXBYTES;

            std::vector<uint8_t> bmp; bmp.resize(bf.bfSize);
            memcpy(bmp.data(), &bf, sizeof(bf));
            memcpy(bmp.data() + sizeof(bf), &bi, sizeof(bi));
            memcpy(bmp.data() + bf.bfOffBits, canvas.data(), PIXBYTES);

            char frameFileName[64] = { 0 };
#if defined(_MSC_VER)
            _snprintf_s(frameFileName, _TRUNCATE, "gif_frame_%d.tmp.bmp", i);
#else
            snprintf(frameFileName, sizeof(frameFileName), "gif_frame_%d.tmp.bmp", i);
#endif

            CGraphicImage* pImage = new CGraphicImage(frameFileName, bmp.data(), (DWORD)bmp.size(), D3DX_FILTER_LINEAR);
            if (!pImage || pImage->IsEmpty()) { SAFE_DELETE(pImage); continue; }

            CGraphicExpandedImageInstance* pInst = CGraphicExpandedImageInstance::New();
            pInst->SetImagePointer(pImage);
            if (pInst->IsEmpty()) { CGraphicExpandedImageInstance::Delete(pInst); continue; }

            pInst->SetPosition(0.f, 0.f);               // a CGifImageBox OnRender úgyis a m_rect-re teszi
            m_ImageVector.push_back(pInst);
            m_FrameDelayMS.push_back(delay_ms);

            // Disposal előkészítés a KÖVETKEZŐ frame-hez
            if (disposal == 3) {              // Previous
                prevCanvas = canvas;
            }
            else if (disposal == 2) {       // Background
                for (int y = 0; y < d.Height; ++y) {
                    int by = CH - 1 - (d.Top + y);
                    uint8_t* p = &canvas[by * STRIDE + (d.Left * BPP)];
                    memset(p, 0x00, d.Width * BPP);
                }
            }
            if (disposal == 3 && !prevCanvas.empty()) { // restore previous
                canvas.swap(prevCanvas);
                prevCanvas.clear();
            }
        }

#if GIFLIB_MAJOR >= 5
        DGifCloseFile(gif, &error);
#else
        DGifCloseFile(gif);
#endif

        return !m_ImageVector.empty();
    }

    bool CGifImageBox::UnloadGif()
    {
        for (auto pImage : m_ImageVector)
            CGraphicExpandedImageInstance::Delete(pImage);

        m_ImageVector.clear();
        return true;
    }

} // namespace UI

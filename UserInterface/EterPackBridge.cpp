// EterPythonLib/EterPackBridge.cpp  — fix LNK2001 a libben
#include "StdAfx.h"
#include "../EterPack/EterPackManager.h"
#include "../EterBase/MappedFile.h"

namespace UI {
    class CEterPackManager {
    public:
        static CEterPackManager& Instance() {
            static CEterPackManager s;
            return s;
        }
        bool Get(CMappedFile& rMappedFile, const char* c_szFileName, const void** pData) {
            return ::CEterPackManager::Instance().Get(rMappedFile, c_szFileName, pData);
        }
        // (opcionálisan) ha kell még: GetFromPack / GetFromFile ugyanígy forwardolva
    };
}
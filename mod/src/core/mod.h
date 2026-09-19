#pragma once
#include <Windows.h>

namespace mst::Mod
{
    void Initialize(HMODULE module);
    void Shutdown(bool processExiting);
}

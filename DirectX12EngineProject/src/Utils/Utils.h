#pragma once

#include <Windows.h>
#include <stdexcept>      // 예외 처리 (선택 사항, HRESULT 확인으로 대체 가능)
#include <filesystem>
#include <iostream>

// __FILEW__ 매크로를 위한 헬퍼
#define WIDEN(x) L##x
#define WIDEN2(x) WIDEN(x)
#define __WFILE__ WIDEN2(__FILE__)

// 커스텀 예외 클래스
class DxException : public std::exception
{
public:
    DxException(HRESULT hr, const wchar_t* functionName, const wchar_t* filename, int lineNumber);

    const char* what() const override
    {
        return m_whatBuffer.c_str();
    }

    HRESULT GetErrorCode() const { return m_errorCode; }

private:
    HRESULT m_errorCode;
    std::string m_whatBuffer;
};

// ThrowIfFailed 매크로
#define ThrowIfFailed(x)                                                       \
{                                                                              \
    HRESULT hr__ = (x);                                                        \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, __WFILE__, __LINE__); }    \
}

// 실행 파일(.exe)이 있는 디렉터리 경로를 반환하는 함수
std::wstring GetExeDirectory();

// wstring을 string으로 변환하는 함수
std::string WstringToString(const std::wstring& wstr);

// HLSL 컴파일된 셰이더 파일(.cso)을 읽어와 바이트코드 벡터로 반환하는 함수
std::vector<byte> ReadShaderBytecode(const std::wstring& filename);

namespace Debug
{
    void CreateConsole();

    inline void Print(const std::wstring msg)
    {
        OutputDebugString((msg + L"\n").c_str());
        std::wcout << msg << std::endl;
    }
}



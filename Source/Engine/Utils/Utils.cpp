#include "Utils.h"
#include <iostream> // for std::cout, std::ios
#include <io.h>     // for _open_osfhandle
#include <fcntl.h>  // for _O_TEXT
#include <string>
#include <comdef.h> // for _com_error
#include <fstream>  // for std::ifstream

DxException::DxException(HRESULT hr, const wchar_t* functionName, const wchar_t* filename, int lineNumber) :
    m_errorCode(hr)
{
    wchar_t* pMsgBuf = nullptr;
    // FormatMessageW 함수를 사용하여 시스템으로부터 HRESULT에 대한 오류 메시지 문자열을 얻어옵니다.
    DWORD nMsgLen = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&pMsgBuf), 0, nullptr);

    // 메시지를 성공적으로 얻어왔는지 확인
    std::wstring error_message = (nMsgLen == 0) ? L"Unknown error." : pMsgBuf;

    // 시스템이 할당한 버퍼를 해제합니다.
    LocalFree(pMsgBuf);

    // 최종 오류 메시지를 wstring으로 조합합니다.
    std::wstring final_msg = 
        std::wstring(functionName) + L"\n\nFile: " + filename + L"\nLine: " + std::to_wstring(lineNumber) + L"\nError: " + error_message;

    // wstring을 C++ 표준 방식으로 string으로 변환합니다.
    // 이 변환 과정은 생성자에서 미리 수행하여 what() 메서드의 부담을 덜어줍니다.
    // wcstombs_s를 사용하는 대신, _com_error를 변환 용도로만 사용하면 더 간편합니다.
    _bstr_t bstr(final_msg.c_str());
    m_whatBuffer = bstr;
}


void Debug::CreateConsole()
{
    // AllocConsole() 함수로 새로운 콘솔 창을 생성합니다.
    if (!AllocConsole())
    {
        // 실패 시 처리 (보통 실패하지 않음)
        return;
    }

    // 표준 입출력(stdin, stdout, stderr)을 새로 만든 콘솔로 리다이렉션합니다.
    // _wfreopen_s 함수를 사용하여 기존 스트림을 새로운 경로("CONIN$", "CONOUT$")로 다시 엽니다.
    // "CONIN$"과 "CONOUT$"은 콘솔의 입력과 출력을 의미하는 특수한 파일 이름입니다.
    FILE* pCin = nullptr;
    FILE* pCout = nullptr;
    FILE* pCerr = nullptr;

    _wfreopen_s(&pCin, L"CONIN$", L"r", stdin);
    _wfreopen_s(&pCout, L"CONOUT$", L"w", stdout);
    _wfreopen_s(&pCerr, L"CONOUT$", L"w", stderr);

    // 3. C++의 std::cout, std::cin 등이 리다이렉션된 C 스트림(stdout, stdin)을
    //    정상적으로 사용하도록 스트림을 초기화/동기화합니다.
    std::ios::sync_with_stdio(true);

    // 4. (선택 사항) 유니코드(wchar_t) 출력을 위해 로케일 설정
    std::wcout.imbue(std::locale("korean"));
    std::wclog.imbue(std::locale("korean"));

    std::cout << "Console Created Successfully!" << std::endl;
}

// 실행 파일(.exe)이 있는 디렉터리 경로를 반환하는 함수
std::filesystem::path GetExeDirectory()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

// 실행 파일(.exe)이 있는 디렉터리 경로를 WString반환하는 함수
std::wstring GetExeDirectoryWstring()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path().wstring();
}

std::string WstringToString(const std::wstring& wstr)
{
    // wstring을 string으로 변환하는 함수
    // 여기서는 간단히 _bstr_t를 사용하여 변환합니다.
    _bstr_t bstr(wstr.c_str());
    return std::string(bstr);
}

std::vector<byte> ReadShaderBytecode(const std::wstring& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if(!file.is_open())
    {
        Debug::Print(L"Failed to open shader file: " + filename);
		throw std::runtime_error("Failed to open shader file. Filename : {}" + WstringToString(filename));
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<byte> buffer(fileSize);

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    return buffer;
}

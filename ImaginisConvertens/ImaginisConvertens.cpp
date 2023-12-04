
#include "dialogue.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CImageOperationDialogue* pDialogue = new CImageOperationDialogue();
    LRESULT iRet = pDialogue->Create(hInstance, MAKEINTRESOURCE(IDD_DIALOG_IMAGE_OPERATION), nullptr);
    delete pDialogue;

    return (int)iRet;
}


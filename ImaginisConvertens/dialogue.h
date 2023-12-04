#ifndef DIALOGUE_H_
#define DIALOGUE_H_

#include <string>

#include <windows.h>

#include "resource.h"
#include "image_operation.h"

class CImageOperationDialogue
{
public:
    LRESULT Create(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent)
    {
        return DialogBoxParam(hInstance, lpTemplateName, hWndParent, (DLGPROC)DialogProc, (LPARAM)this);
    }
private:
    HWND m_hWnd = nullptr;
    static LRESULT CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnInit(HWND hWnd);
    LRESULT OnClose(WPARAM wParam);
    LRESULT OnCommand(WPARAM wParam);

    CImageOperation* m_image_operation = nullptr;

    void SetControlItemRange();
    void MakeComboBoxList();
    std::vector<std::wstring> m_threshold_list = { L"THRESH_BINARY", L"THRESH_BINARY_INV", L"THRESH_TRUNC", L"THRESH_TOZERO", L"THRESH_TOZERO_INV" };
    std::vector<std::wstring> m_page_list = { L"Dextrorsum", L"Sinistrorsum"};
    void ApplyFilterVariables();
    unsigned int GetPageNumber();
};

#endif
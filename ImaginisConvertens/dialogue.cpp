
#include "dialogue.h"
#include "file_operation.h"

LRESULT CALLBACK CImageOperationDialogue::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, DWLP_USER, lParam);
	}

	CImageOperationDialogue* pThis = reinterpret_cast<CImageOperationDialogue*>(GetWindowLongPtr(hWnd, DWLP_USER));
	if (pThis != nullptr)
	{
		return pThis->HandleMessage(hWnd, uMsg, wParam, lParam);
	}

	return FALSE;
}

LRESULT CImageOperationDialogue::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInit(hWnd);
	case WM_CLOSE:
		return OnClose(wParam);
	case WM_COMMAND:
		return OnCommand(wParam);
	}
	return FALSE;
}

LRESULT CImageOperationDialogue::OnInit(HWND hWnd)
{
	m_hWnd = hWnd;

	wchar_t* folder = SelectWorkingFolder();
	if (folder != nullptr)
	{
		m_image_operation = new CImageOperation(folder);
		SetControlItemRange();
		MakeComboBoxList();
		::CoTaskMemFree(folder);
	}
	else
	{
		PostMessage(m_hWnd, WM_CLOSE, 0, 0);
	}

	return TRUE;
}

LRESULT CImageOperationDialogue::OnClose(WPARAM wParam)
{
	delete m_image_operation;
	m_image_operation = nullptr;
	EndDialog(m_hWnd, wParam);
	return FALSE;
}

LRESULT CImageOperationDialogue::OnCommand(WPARAM wParam)
{
	if (m_image_operation == nullptr)return FALSE;
	switch (LOWORD(wParam)) {
	case IDC_TEST:
		ApplyFilterVariables();
		m_image_operation->TestSingleImage(GetPageNumber());
		break;
	case IDC_EXECUTE:
		ApplyFilterVariables();
		m_image_operation->ExecuteThresholding();
		break;
	case IDC_ADAPTIVE_TEST:
		ApplyFilterVariables();
		m_image_operation->TestAdaptively(GetPageNumber());
		break;
	case IDC_CUT:
		ApplyFilterVariables();
		m_image_operation->ExecuteCutting();
		break;
	case IDC_MANUAL_CROP:
		m_image_operation->ManuallyCrop();
		break;
	case IDC_CROP:
		ApplyFilterVariables();
		m_image_operation->ExecuteCropping();
		break;
	case IDC_CROP_TEST:
		ApplyFilterVariables();
		m_image_operation->TestCroppingWhiteSpace(GetPageNumber());
		break;
	case IDC_BINARISE:
		ApplyFilterVariables();
		m_image_operation->ExecuteCompressing();
		break;
	case IDC_FIXED_CROP:
		m_image_operation->ExecuteFixedTrimming();
		break;
	}
	return TRUE;
}
/*入力値制限*/
void CImageOperationDialogue::SetControlItemRange()
{
	/*Spin control*/
	unsigned long long page = m_image_operation->GetElementsQuantity();
	SendDlgItemMessage(m_hWnd, IDC_SPIN_PAGE, UDM_SETRANGE, 0, MAKELONG(page, 0));
	SendDlgItemMessage(m_hWnd, IDC_SPIN_PAGE, UDM_SETPOS, 0, 1);

	SendDlgItemMessage(m_hWnd, IDC_SPIN_THRESHOLD, UDM_SETRANGE, 0, MAKELONG(255, 0));
	SendDlgItemMessage(m_hWnd, IDC_SPIN_THRESHOLD, UDM_SETPOS, 0, 153);

	SendDlgItemMessage(m_hWnd, IDC_SPIN_MAX, UDM_SETRANGE, 0, MAKELONG(255, 0));
	SendDlgItemMessage(m_hWnd, IDC_SPIN_MAX, UDM_SETPOS, 0, 255);

	SendDlgItemMessage(m_hWnd, IDC_SPIN_BLOCK, UDM_SETRANGE, 0, MAKELONG(99, 0));
	SendDlgItemMessage(m_hWnd, IDC_SPIN_BLOCK, UDM_SETPOS, 0, 5);
	SendDlgItemMessage(m_hWnd, IDC_SPIN_CONSTANT, UDM_SETRANGE, 0, MAKELONG(99, -99));
	SendDlgItemMessage(m_hWnd, IDC_SPIN_CONSTANT, UDM_SETPOS, 0, 5);
	SendDlgItemMessage(m_hWnd, IDC_SPIN_BLUR, UDM_SETRANGE, 0, MAKELONG(99, -99));
	SendDlgItemMessage(m_hWnd, IDC_SPIN_BLUR, UDM_SETPOS, 0, 5);

	SendDlgItemMessage(m_hWnd, IDC_SPIN_SHARPNESS, UDM_SETRANGE, 0, MAKELONG(10, 0));
	SendDlgItemMessage(m_hWnd, IDC_SPIN_SHARPNESS, UDM_SETPOS, 0, 2);
	SendDlgItemMessage(m_hWnd, IDC_SPIN_BRIGHTNESS, UDM_SETRANGE, 0, MAKELONG(20, 0));
	SendDlgItemMessage(m_hWnd, IDC_SPIN_BRIGHTNESS, UDM_SETPOS, 0, 9);

	/*Edit control*/
	SendDlgItemMessage(m_hWnd, IDC_EDIT_PAGE, EM_LIMITTEXT, 4, 0);
	SendDlgItemMessage(m_hWnd, IDC_EDIT_THRESHOLD, EM_LIMITTEXT, 3, 0);
	SendDlgItemMessage(m_hWnd, IDC_EDIT_MAX, EM_LIMITTEXT, 3, 0);

	SendDlgItemMessage(m_hWnd, IDC_EDIT_BLOCK, EM_LIMITTEXT, 2, 0);
	SendDlgItemMessage(m_hWnd, IDC_EDIT_CONSTANT, EM_LIMITTEXT, 2, 0);
	SendDlgItemMessage(m_hWnd, IDC_EDIT_BLUR, EM_LIMITTEXT, 2, 0);

	SendDlgItemMessage(m_hWnd, IDC_EDIT_SHARPNESS, EM_LIMITTEXT, 2, 0);
	SendDlgItemMessage(m_hWnd, IDC_EDIT_BRIGHTNESS, EM_LIMITTEXT, 2, 0);

}
/*コンボボックスの項目名挿入*/
void CImageOperationDialogue::MakeComboBoxList()
{

	for (size_t i = 0; i < m_threshold_list.size(); ++i) 
	{
		SendDlgItemMessage(m_hWnd, IDC_COMBO_THRESHOLD, CB_INSERTSTRING, i, reinterpret_cast<LPARAM>(m_threshold_list.at(i).data()));
	}
	SendDlgItemMessage(m_hWnd, IDC_COMBO_THRESHOLD, CB_SETCURSEL, 3, 0);

	for (size_t i = 0; i < m_page_list.size(); ++i)
	{
		SendDlgItemMessage(m_hWnd, IDC_COMBO_CUT, CB_INSERTSTRING, i, reinterpret_cast<LPARAM>(m_page_list.at(i).data()));
	}
	SendDlgItemMessage(m_hWnd, IDC_COMBO_CUT, CB_SETCURSEL, 0, 0);

}
/*変数設定*/
void CImageOperationDialogue::ApplyFilterVariables()
{
	if (m_image_operation != nullptr)
	{
		m_image_operation->m_threshold_value = (int)SendDlgItemMessage(m_hWnd, IDC_SPIN_THRESHOLD, UDM_GETPOS32, 0, 0);
		m_image_operation->threshold_kind = (int)SendDlgItemMessage(m_hWnd, IDC_COMBO_THRESHOLD, CB_GETCURSEL, 0, 0);
		m_image_operation->max_value = (int)SendDlgItemMessage(m_hWnd, IDC_SPIN_MAX, UDM_GETPOS32, 0, 0);

		m_image_operation->m_sharpness = (int)SendDlgItemMessage(m_hWnd, IDC_SPIN_SHARPNESS, UDM_GETPOS32, 0, 0);
		m_image_operation->m_gamma = (double)SendDlgItemMessage(m_hWnd, IDC_SPIN_BRIGHTNESS, UDM_GETPOS32, 0, 0) / 10.0;

		m_image_operation->SetPageDirection((int)SendDlgItemMessage(m_hWnd, IDC_COMBO_CUT, CB_GETCURSEL, 0, 0));

		/*順応処理変数*/
		int block = (int)SendDlgItemMessage(m_hWnd, IDC_SPIN_BLOCK, UDM_GETPOS32, 0, 0);
		int blur = (int)SendDlgItemMessage(m_hWnd, IDC_SPIN_BLUR, UDM_GETPOS32, 0, 0);
		if (!(block % 2))++block;
		if (!(blur % 2))++blur;
		m_image_operation->m_block_value = block;
		m_image_operation->m_blur_value = blur;
		m_image_operation->m_adaptive_constant = (int)SendDlgItemMessage(m_hWnd, IDC_SPIN_CONSTANT, UDM_GETPOS32, 0, 0);
	}

}
/*指定頁数の取得*/
unsigned int CImageOperationDialogue::GetPageNumber()
{
	return (unsigned int)SendDlgItemMessage(m_hWnd, IDC_SPIN_PAGE, UDM_GETPOS32, 0, 0);
}
#define _CRT_SECURE_NO_WARNINGS

#include <afxwin.h>
#include <afxext.h>
#include <afxcontrolbars.h>

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else // x64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

class CMFCApplication1App : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

#include "resource.h"
class dialog : public CDialogEx
{
	DECLARE_DYNAMIC(dialog)

#ifdef AFX_DESIGN_TIME // this needs by dialog editor
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
public:
	//long m_maxbuf;
	int m_vst_buf_size;
	float m_freq;
	CComboBox m_asiolist;
	CString m_asio_name;
	CString m_asio_status;
	afx_msg void OnBnClickedButton1();
	afx_msg void OnSelchangeCombo1();
	int m_asio_buf_size;
	CString m_asio_message;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButton3();
	afx_msg void OnClickedRadioAsiobufx2();
	int m_ringsize_int;
	CButton m_ringsize_btn2;
	CButton m_ringsize_btn3;
	CButton m_ringsize_btn1;
	BOOL m_is_stereo;
	CComboBox m_input_num_1;
	CComboBox m_input_num_2;
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnBnClickedCheck1();
	CString m_asio_info;
	CStatic m_ring_groupbox;
};

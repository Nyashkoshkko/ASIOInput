#include "dialog.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(dialog, CDialogEx)
void dialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT2, m_vst_buf_size);
    DDX_Text(pDX, IDC_EDIT3, m_freq);
    DDX_Control(pDX, IDC_COMBO1, m_asiolist);
    DDX_Text(pDX, IDC_STATIC_ASIONAME, m_asio_name);
    DDX_Text(pDX, IDC_STATIC_ASIOSTATUS, m_asio_status);
    DDX_Text(pDX, IDC_STATIC_ASIOBUF, m_asio_buf_size);
    DDX_Text(pDX, IDC_STATIC_ERROR_MESSAGE, m_asio_message);
    DDX_Radio(pDX, IDC_RADIO_ASIOBUFX2, m_ringsize_int);
    DDX_Control(pDX, IDC_RADIO_ASIOBUFX3, m_ringsize_btn2);
    DDX_Control(pDX, IDC_RADIO_ASIOBUFX4, m_ringsize_btn3);
    DDX_Control(pDX, IDC_RADIO_ASIOBUFX2, m_ringsize_btn1);
    DDX_Check(pDX, IDC_CHECK1, m_is_stereo);
    DDX_Control(pDX, IDC_COMBO2, m_input_num_1);
    DDX_Control(pDX, IDC_COMBO3, m_input_num_2);
    DDX_Text(pDX, IDC_STATIC_ASIO_INFO, m_asio_info);
    DDX_Control(pDX, IDC_STATIC_RING_GROUP, m_ring_groupbox);
}
BEGIN_MESSAGE_MAP(dialog, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON1, &dialog::OnBnClickedButton1)
    ON_CBN_SELCHANGE(IDC_COMBO1, &dialog::OnSelchangeCombo1)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_BUTTON3, &dialog::OnBnClickedButton3)
    ON_BN_CLICKED(IDC_RADIO_ASIOBUFX2, &dialog::OnClickedRadioAsiobufx2)
    ON_BN_CLICKED(IDC_RADIO_ASIOBUFX3, &dialog::OnClickedRadioAsiobufx2)
    ON_BN_CLICKED(IDC_RADIO_ASIOBUFX4, &dialog::OnClickedRadioAsiobufx2)
    ON_CBN_SELCHANGE(IDC_COMBO2, &dialog::OnCbnSelchangeCombo2)
    ON_CBN_SELCHANGE(IDC_COMBO3, &dialog::OnCbnSelchangeCombo3)
    ON_BN_CLICKED(IDC_CHECK1, &dialog::OnBnClickedCheck1)
END_MESSAGE_MAP()

CMFCApplication1App theApp;
dialog dlg;

#define PTRSIZE sizeof(void *)
struct SVSTPlugin
{
    char data1[4];

    void* (*RequestFromHost) (void* param1, int requestCode, int param2, void* param3, void* param4, float param5);

    char data2[(2 * 4) + (3 * PTRSIZE)];

    int InputCount;
    int OutputCount;
    int PluginProperties;

    char data3[(6 * 4) + (4 * PTRSIZE)];

    void (*UpdateBufferData) (void* param1, float** buffersIn, float** buffersOut, int bufSize);
    
    char data4[56 + PTRSIZE];
};
SVSTPlugin VSTPlugin;
 
extern "C" { __declspec(dllexport) SVSTPlugin* VSTPluginMain(void* audioMaster) { return &VSTPlugin; } } // extern "C" // HOST CALL THIS FUNC FOR FIND

void ASIOUnload(); // proto
void ASIORun();
void ASIOStop();

//#define DEBUG_CONSOLE

int run = 0; // is child asio driver is runed
int reset_request = 0, ring_sync_request = 0, ring_can_operate = 0, ring_schedule_sync = 0;

void* VSTPluginCall_RequestFromHost(void* param1, int requestCode, int param2, void* param3, void* param4, float param5)
{
    int ret = 0;
    switch (requestCode)
    {
    case 19: // idle process call (ordinary used for gui refresh)
        if(dlg.IsWindowVisible())
        {
            dlg.UpdateData(false);
        }
        if(reset_request)
        {
            reset_request = 0;
            dlg.OnSelchangeCombo1();
        }
        break;

    case 10: // new sample rate
        if((run) && (param5 != dlg.m_freq)) dlg.OnSelchangeCombo1();
        dlg.m_freq = param5;
        break;

    case 11: // new default vst buffer size
        dlg.m_vst_buf_size = (int)param3; // for run guest asio without host asio enabled
        if((run) && ((int)param3 != dlg.m_vst_buf_size)) dlg.OnSelchangeCombo1();
        break;

    case 14: // show vst window
        dlg.SetParent(CWnd::FromHandle((HWND)param4));
        dlg.ShowWindow(SW_SHOW);
        ret = 1;
        break;

    case 15: // hide vst window
        dlg.ShowWindow(SW_HIDE);
        dlg.SetParent(NULL);
        break;

    case 13: // get vst window size
    {
        static short VSTWindowSizes[4]; // 0 -  up, 1 - left, 2 - down, 3 - right
        RECT rect_loc; dlg.GetClientRect(&rect_loc); VSTWindowSizes[1] = rect_loc.left; VSTWindowSizes[0] = rect_loc.top; VSTWindowSizes[3] = rect_loc.right; VSTWindowSizes[2] = rect_loc.bottom;
        short** rect_int = (short**)param4; *rect_int = VSTWindowSizes;
        ret = 1;
        break; 
    }

    case 1: // close vst plugin processing
        ASIOUnload();
        break;

    case 0: // init vst plugin processing
    {
        static int firstRun = 1;
        if(!firstRun)
        {
            dlg.OnSelchangeCombo1();
        }
        firstRun = 0;
        break;
    }

    case 12: // vst plugin processing pause and resume	
        if(run)
        {
            if(param3 == 0) // vst processing pause
            {
                ASIOStop();
            }
            else // vst processing resume
            {
                ASIORun();
            }
        }
        break;
    }

    return (void*)ret;
}

void VSTPluginCall_UpdateBufferData(void* param1, float** buffersIn, float** buffersOut, int bufSize);
void* ASIOHostCall_UpdateBufferDataEx(void* param1, int index, int param2);

#include <vector>
#include <string>

std::vector<GUID> drivers;

struct SASIOChannel
{
    int Number;
    int Direction; // 0 - output, 1 - input
    int Online;
    int GroupNumber;
    int SampleType;
    char Name[32];
};

struct SASIOBuffer
{
    int Direction; // 0 - output, 1 - input
    int LinkedChannelNumber;
    void* Buffer[2]; // double buff
};

struct SASIOHostFuncs
{
    void (*UpdateBufferData) (int index, int param1);
    void (*SetNewSampleRate) (double newSampleRate);
    int (*RequestToHost) (int requestCode, int param1, void* param2, void* param3);
    void * (*UpdateBufferDataEx) (void* param1, int index, int param2);
};

interface IASIODriver : public IUnknown
{
    virtual int ASIOInit(void* ptr) = 0; // only in this function: 1 if init ok, not 0 (other funcs ok=0)
    virtual void func1() = 0; virtual void func2() = 0;
    virtual void ASIOGetMessage(char* message) = 0;
    virtual int ASIORun() = 0;
    virtual int ASIOStop() = 0;
    virtual int ASIOGetChannelsCount(int* inputCount, int* outputCount) = 0;
    virtual void func3() = 0;
    virtual int ASIOGetBufferSizes(int* minBufSize, int* maxBufSize, int* curBufSize, int* bufSizeDivider) = 0;
    virtual void func4() = 0; virtual void func5() = 0;
    virtual int ASIOSetNewSampleRate(double newSampleRate) = 0;
    virtual void func6() = 0; virtual void func7() = 0; virtual void func8() = 0;
    virtual int ASIOGetChannel(struct SASIOChannel* ASIOChannel) = 0;
    virtual int ASIOBuffersNew(struct SASIOBuffer* ASIOBuffers, int bufCount, int bufSize, struct SASIOHostFuncs* hostFuncs) = 0;
    virtual int ASIOBuffersDelete() = 0;
    virtual int ASIOShowControlPanel() = 0;
    virtual void func9() = 0; virtual void func10() = 0;
};
IASIODriver* driver = 0;
struct SASIOBuffer ASIOBuffers[2];
struct SASIOHostFuncs ASIOHostFuncs;

void ASIOHostCall_SetNewSampleRate(double newSampleRate)
{
    reset_request = 1; // asio driver want new sample rate - schedue reloading it instead applying new sample rate
}
int ASIOHostCall_RequestToHost(int requestCode, int param1, void* param2, void* param3)
{
    switch(requestCode)
    {
    case 1: if(param1 == 2 || param1 == 3) return 1; break;  // request code 1 - 'is host support request codes'
    case 2: return 2; // request code 2 - get supported by host asio version - return 2
    case 3: reset_request = 1; return 1; // request code 3 - asio driver has changed settings - schedue reloading it
    }
    return 0;
}
void ASIOHostCall_UpdateBufferData(int index, int param1)
{
    ASIOHostCall_UpdateBufferDataEx(NULL, index, param1);
}


CSingleLock* singleLock; // mutex

HANDLE SignalWriteSamples = NULL;
HANDLE SignalWriteDone = NULL;

int synch_type = 0; // 0 - synch by event, 1 - synch by ring buff

int ringsize_x1 = 0;
int ring_mult = 3;
float* input[2] = { 0, 0 };
int ring_size = (256 * 4);
int ptr_write = 0, ptr_read = 0; // 256 better latency

int ch_num[2] = { 0, 1 };

void ASIORun()
{
    if(driver)
    {
        driver->ASIORun();
    }
}

void ASIOStop()
{
    if(driver)
    {
        driver->ASIOStop();
    }
}

// when this vst dll loads
BOOL CMFCApplication1App::InitInstance()
{
    #ifdef DEBUG_CONSOLE
    ::AllocConsole();
    freopen("CONOUT$", "wt", stdout);
    #endif // DEBUG_CONSOLE

    // create mutex
    singleLock = new CSingleLock(new CMutex(FALSE, "ASIOInputVSTASIOSynchMutex", NULL));

    // create signal
    SignalWriteSamples = CreateEvent(NULL, FALSE, FALSE, "ASIOInputWriteSamplesSignal");
    SignalWriteDone = CreateEvent(NULL, FALSE, FALSE, "ASIOInpuWriteDoneSignal");

    // create dialog window
    dlg.Create(IDD_DIALOG1);
    dlg.m_asio_status = "NOT LOADED";
    dlg.m_ringsize_int = 1; //0 - x2, 1 - x3, 2 - x4; x3 ring mult is default
    dlg.m_is_stereo = 1;
    dlg.m_input_num_1.AddString("0 - Input 0");
    dlg.m_input_num_1.AddString("1 - Input 1");
    dlg.m_input_num_1.SetCurSel(0);
    dlg.m_input_num_2.AddString("0 - Input 0");
    dlg.m_input_num_2.AddString("1 - Input 1");
    dlg.m_input_num_2.SetCurSel(1);
    dlg.SetTimer(IDD_TIMER_1_SEC, 1000, NULL);

    // create vst struct for host
    memset(&VSTPlugin, 0, sizeof(struct SVSTPlugin));
    VSTPlugin.InputCount = 0;
    VSTPlugin.OutputCount = 2;
    VSTPlugin.PluginProperties = 1 | (1 << 4) | (1 << 8); // bit 0 - plugin with editor; bit 4 - support UpdateBufferData() call; bit 8 - host will reserve dedicated mixer line for plugin
    VSTPlugin.RequestFromHost = VSTPluginCall_RequestFromHost;
    VSTPlugin.UpdateBufferData = VSTPluginCall_UpdateBufferData;

    // asio driver callers
    ASIOHostFuncs.RequestToHost = &ASIOHostCall_RequestToHost;
    ASIOHostFuncs.UpdateBufferData = &ASIOHostCall_UpdateBufferData;
    ASIOHostFuncs.UpdateBufferDataEx = &ASIOHostCall_UpdateBufferDataEx;
    ASIOHostFuncs.SetNewSampleRate = &ASIOHostCall_SetNewSampleRate;

    // load asio drivers list from registry
    CRegKey key;
    if (key.Open(HKEY_LOCAL_MACHINE, "SOFTWARE\\ASIO", KEY_READ) == 0)
    {
        int i = 0;
        char param[500];
        int len = sizeof(param);
        while (key.EnumKey(i, param, (LPDWORD)&len) == 0)
        {
            CRegKey subkey;
            if (subkey.Open(HKEY(key), param, KEY_READ) == 0)
            {
                CString name;
                GUID clsid;

                char desc[500];
                unsigned long len = sizeof(desc);
                if (subkey.QueryStringValue("Description", desc, &len) == 0)
                {
                    name = CString(desc);
                }
                else
                {
                    name = CString(param);
                }

                if (subkey.QueryGUIDValue("CLSID", clsid) == 0)
                {
                    drivers.push_back(clsid); // add to driver guids list
                    dlg.m_asiolist.AddString(name); // add to dialog
                }

                subkey.Close();
            }

            len = sizeof(param);
            i++;
        }
        key.Close();
    }

    return true;
}

// when this vst dll unloads
int CMFCApplication1App::ExitInstance()
{
    synch_type = 1;
    Sleep(100);
    
    // if driver loaded
    ASIOUnload();
    
    // delete dialog window
    dlg.KillTimer(IDD_TIMER_1_SEC);
    dlg.DestroyWindow();

    // delete mutex
    singleLock->Unlock();
    delete singleLock;

    // delete signal
    CloseHandle(SignalWriteSamples);
    CloseHandle(SignalWriteDone);

    delete input[0];
    delete input[1];

    #ifdef DEBUG_CONSOLE
    fclose(stdout);
    ::FreeConsole();
    #endif // DEBUG_CONSOLE

    return 0;
}

// clicked control panel button
void dialog::OnBnClickedButton1()
{
    if(driver)
    {
        driver->ASIOShowControlPanel();
    }
}


// host call this vst callback
float** vstoutputbuffers;
int vstsamplecount;
void VSTPluginCall_UpdateBufferData(void* param1, float** buffersIn, float** buffersOut, int bufSize)
{
    dlg.m_vst_buf_size = bufSize;
    
    if(!run)
    {
        for(int i = 0; i < bufSize; i++)
        {
            buffersOut[0][i] = 0.f;
            buffersOut[1][i] = 0.f;
        }
    }
    else
    {
        if(synch_type == 1) // 1 - ring
        {
            singleLock->Lock();

            if(ring_can_operate)
            {
                for(int i = 0; i < bufSize; i++)
                {
                    if(ptr_read == (ring_mult * ringsize_x1)) ptr_read = 0; // ring buffer
                    buffersOut[0][i] = input[0][ptr_read];
                    if(dlg.m_is_stereo)
                    {
                        buffersOut[1][i] = input[1][ptr_read];
                    }
                    else
                    {
                        buffersOut[1][i] = buffersOut[0][i];
                    }
                    ptr_read++;
                }
            }
            else
            {
                for(int i = 0; i < bufSize; i++)
                {
                    buffersOut[0][i] = 0.f;
                    buffersOut[1][i] = 0.f;
                }
            }

            singleLock->Unlock();
        }

        if(synch_type == 0) // 0 - event
        {
            vstoutputbuffers = buffersOut;
            vstsamplecount = bufSize;
            SetEvent(SignalWriteSamples);
            WaitForSingleObject(SignalWriteDone, 50);
        }
    }

    return;
}

#define INT32S_TO_DOUBLE 4294967296.

// child asio driver this call vst callback
void* ASIOHostCall_UpdateBufferDataEx(void* param1, int index, int param2)
{
    if(synch_type == 1) // 1 - ring
    {
        singleLock->Lock();

        if(ring_sync_request)
        {
            // make new write and read pointers for ring buf operation
            ptr_write = ringsize_x1;
            ptr_read = 0;

            ring_can_operate = 1;
            ring_sync_request = 0;
        }

        if(ring_can_operate)
        {
            for(int i = 0; i < dlg.m_asio_buf_size; i++)
            {
                if(ptr_write == (ring_mult * ringsize_x1)) ptr_write = 0; // ring buffer
                input[0][ptr_write] = (float)((double)(((int*)(ASIOBuffers[0].Buffer[index]))[i]) / INT32S_TO_DOUBLE);
                if(dlg.m_is_stereo)
                {
                    input[1][ptr_write] = (float)((double)(((int*)(ASIOBuffers[1].Buffer[index]))[i]) / INT32S_TO_DOUBLE);
                }
                ptr_write++;
            }
        }

        singleLock->Unlock();
    }

    if(synch_type == 0) // 0 - event
    {
        if(WaitForSingleObject(SignalWriteSamples, 50) == 0)
        {
            for(int i = 0; i < vstsamplecount; i++)
            {
                vstoutputbuffers[0][i] = (float)((double)(((int*)(ASIOBuffers[0].Buffer[index]))[i]) / INT32S_TO_DOUBLE);
                if(dlg.m_is_stereo)
                {
                    vstoutputbuffers[1][i] = (float)((double)(((int*)(ASIOBuffers[1].Buffer[index]))[i]) / INT32S_TO_DOUBLE);
                }
                else
                {
                    vstoutputbuffers[1][i] = vstoutputbuffers[0][i];
                }
            }
            SetEvent(SignalWriteDone);
        }
    }
    
    return 0;
}

void ASIOUnload()
{
    if(driver != 0)
    {
        ring_can_operate = 0;
        dlg.m_asio_status = "NOT LOADED";
        synch_type = 1;
        run = 0;
        driver->ASIOStop();
        Sleep(100);
        driver->ASIOBuffersDelete();
        driver->Release();
        driver = 0;
        CoUninitialize(); // this call must be in EVERY thread whitch work with COM objects
    }
}

// current asio driver in list changed
void dialog::OnSelchangeCombo1()
{
    int asioidx = m_asiolist.GetCurSel();
    if(asioidx < 0) return;
    m_asiolist.GetLBText(asioidx, m_asio_name);

    int ret = -40;

    ASIOUnload();
    dlg.m_asio_message = "";
    CoInitialize(0);
    if((ret = CoCreateInstance(drivers[asioidx], 0, CLSCTX_INPROC_SERVER, drivers[asioidx], (LPVOID*)&driver)) == 0)
    {
        if(driver->ASIOInit(0))
        {
            int asio_cnt_input, asio_cnt_output;
            if((ret = driver->ASIOGetChannelsCount(&asio_cnt_input, &asio_cnt_output)) == 0)
            {
                if((ret = driver->ASIOSetNewSampleRate(dlg.m_freq)) == 0)
                {
                    int asiobuf_min, asiobuf_max, asiobuf_div;
                    if((ret = driver->ASIOGetBufferSizes(&asiobuf_min, &asiobuf_max, &m_asio_buf_size, &asiobuf_div)) == 0)
                    {
                        for(int i = 0; i < (m_is_stereo + 1); i++) // input channels count, interact with dlg
                        {
                            ASIOBuffers[i].Direction = 1; // 0 - output, 1 - input
                            ASIOBuffers[i].LinkedChannelNumber = ch_num[i]; // choose from dlg
                            ASIOBuffers[i].Buffer[0] = 0;
                            ASIOBuffers[i].Buffer[1] = 0;
                        }

                        synch_type = 0; // 0 - event, 1 - ring
                        m_ringsize_btn1.ShowWindow(false);
                        m_ringsize_btn2.ShowWindow(false);
                        m_ringsize_btn3.ShowWindow(false);
                        m_ring_groupbox.ShowWindow(false);
                        if(m_vst_buf_size != m_asio_buf_size)
                        {
                            m_asio_message.Format("Change buf to %i samples in control panel", m_vst_buf_size);
                            synch_type = 1; // 0 - event, 1 - ring
                            m_ringsize_btn1.ShowWindow(true);
                            m_ringsize_btn2.ShowWindow(true);
                            m_ringsize_btn3.ShowWindow(true);
                            m_ring_groupbox.ShowWindow(true);
                        }

                        if((ret = driver->ASIOBuffersNew(ASIOBuffers, (m_is_stereo + 1), m_asio_buf_size, &ASIOHostFuncs)) == 0) // must be between min and max, instead count of asio callback per second will incorrect
                        {
                            m_input_num_1.ResetContent();
                            m_input_num_2.ResetContent();
                            int isSampleFormatSupported = 1;
                            for(int i = 0; i < asio_cnt_input; i++)
                            {
                                struct SASIOChannel channel;
                                channel.Number = i;
                                channel.Direction = 1; // 0 - output, 1 - input
                                driver->ASIOGetChannel(&channel);

                                if((ret = channel.SampleType) != 18) // 18 - int32 lsb
                                {
                                    isSampleFormatSupported = 0;
                                }

                                CString str;
                                str.Format("%i - %s", channel.Number, channel.Name);
                                m_input_num_1.AddString(str);
                                m_input_num_2.AddString(str);
                            }
                            m_input_num_1.SetCurSel(ch_num[0]);
                            m_input_num_2.SetCurSel(ch_num[1]);

                            if(isSampleFormatSupported)
                            {
                                // create ring buffer for data from asio chile driver to vst parent samples
                                ringsize_x1 = (m_asio_buf_size > m_vst_buf_size) ? m_asio_buf_size : m_vst_buf_size;
                                if(input[0]) delete input[0];
                                if(input[1]) delete input[1];
                                ring_size = 4 * ringsize_x1;
                                input[0] = new float[ring_size];
                                input[1] = new float[ring_size];
                                for(int i = 0; i < ring_size; i++)
                                {
                                    input[0][i] = 0.f;
                                    input[1][i] = 0.f;
                                }

                                ring_can_operate = 0;
                                ring_schedule_sync = 1;

                                // run driver
                                if(driver->ASIORun() == 0)
                                {
                                    run = 1;
                                    dlg.m_asio_status = "LOADED";
                                    dlg.m_asio_info = "RUN OK";
                                    return;
                                }
                                else
                                {
                                    dlg.m_asio_info = "ASIORun() FAILED";
                                }
                            }
                            else
                            {
                                driver->ASIOBuffersDelete();
                                m_asio_info.Format("sample format not supported: %i", ret);
                            }
                        }
                        else
                        {
                            m_asio_info.Format("ASIOBuffersNew() FAILED (%i)", ret);
                        }
                    }
                    else
                    {
                        dlg.m_asio_status = "LOADED";
                        m_asio_info.Format("ASIOGetBufferSizes() FAILED (%i)", ret);
                        m_asio_message = "try change settings in contr.panel";
                        return;
                    }
                }
                else
                {
                    m_asio_info.Format("ASIOSetNewSampleRate(%i) FAILED(%i)", (int)dlg.m_freq, ret);
                    m_asio_message = "Select another sample rate in host";
                }
            }
            else
            {
                m_asio_info.Format("ASIOGetChannelsCount() FAILED (%i)", ret);
            }
        }
        else
        {
            m_asio_info = "ASIOInit() FAILED";
        }
    }
    else
    {
        m_asio_info.Format("LOAD FAILED (%i)", ret);
    }

    run = 0;
    char message[500]; message[0] = 0;
    if(driver) driver->ASIOGetMessage(message);
    if(message[0]) dlg.m_asio_message = message;
    if(driver) driver->Release();
    driver = 0;
    CoUninitialize();
}

void dialog::OnTimer(UINT_PTR nIDEvent)
{
    if(ring_schedule_sync)
    {
        singleLock->Lock();

        ring_sync_request = 1;
        ring_can_operate = 0;

        singleLock->Unlock();

        ring_schedule_sync = 0;
    }
}

void dialog::OnBnClickedButton3()
{
    m_asio_info = "";
    ASIOUnload();
}

void dialog::OnClickedRadioAsiobufx2()
{
    UpdateData();

    ring_mult = dlg.m_ringsize_int + 2;
    ring_can_operate = 0;
    ring_schedule_sync = 1;
}

void dialog::OnCbnSelchangeCombo2()
{
    ch_num[0] = m_input_num_1.GetCurSel();
    if(run) OnSelchangeCombo1();
}

void dialog::OnCbnSelchangeCombo3()
{
    ch_num[1] = m_input_num_2.GetCurSel();
    if(run) OnSelchangeCombo1();
}

void dialog::OnBnClickedCheck1() // m_is_stereo
{
    UpdateData();
    if(run) OnSelchangeCombo1();
}

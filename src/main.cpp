#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <shellapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "advapi32.lib")

static const USHORT VID=0x046D, PID=0xC336, HID_USAGE_PAGE_VALUE=0xFF43, HID_USAGE_VALUE=0x0602;
static const BYTE R=0x3E, G=0x31, B=0x00;

static bool startup_mode(){
    int n=0; LPWSTR* a=CommandLineToArgvW(GetCommandLineW(),&n); bool r=false;
    if(a){ for(int i=1;i<n;i++) if(!_wcsicmp(a[i],L"--startup")) r=true; LocalFree(a); }
    return r;
}

static bool install_self(){
    wchar_t self[MAX_PATH], local[MAX_PATH];
    if(!GetModuleFileNameW(nullptr,self,MAX_PATH)) return false;
    if(!GetEnvironmentVariableW(L"LOCALAPPDATA",local,MAX_PATH)) return false;
    std::wstring dir=std::wstring(local)+L"\\G213Color";
    std::wstring dst=dir+L"\\G213Color.exe";
    CreateDirectoryW(dir.c_str(),nullptr);
    if(_wcsicmp(self,dst.c_str()) && !CopyFileW(self,dst.c_str(),FALSE)) return false;
    std::wstring cmd=L"\""+dst+L"\" --startup";
    HKEY k=nullptr;
    if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,nullptr,0,KEY_SET_VALUE,nullptr,&k,nullptr)!=ERROR_SUCCESS) return false;
    LONG rc=RegSetValueExW(k,L"G213Color",0,REG_SZ,(const BYTE*)cmd.c_str(),(DWORD)((cmd.size()+1)*sizeof(wchar_t)));
    RegCloseKey(k); return rc==ERROR_SUCCESS;
}

static bool set_color(std::wstring& why){
    GUID guid; HidD_GetHidGuid(&guid);
    HDEVINFO ds=SetupDiGetClassDevsW(&guid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if(ds==INVALID_HANDLE_VALUE){ why=L"Cannot enumerate HID devices."; return false; }
    bool saw=false, sawUsage=false;
    for(DWORD i=0;;i++){
        SP_DEVICE_INTERFACE_DATA id{}; id.cbSize=sizeof(id);
        if(!SetupDiEnumDeviceInterfaces(ds,nullptr,&guid,i,&id)){ if(GetLastError()==ERROR_NO_MORE_ITEMS) break; else continue; }
        DWORD need=0; SetupDiGetDeviceInterfaceDetailW(ds,&id,nullptr,0,&need,nullptr);
        if(!need) continue;
        std::vector<BYTE> mem(need);
        auto* d=(SP_DEVICE_INTERFACE_DETAIL_DATA_W*)mem.data(); d->cbSize=sizeof(*d);
        if(!SetupDiGetDeviceInterfaceDetailW(ds,&id,d,need,nullptr,nullptr)) continue;
        HANDLE h=CreateFileW(d->DevicePath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
        if(h==INVALID_HANDLE_VALUE) continue;
        HIDD_ATTRIBUTES at{}; at.Size=sizeof(at);
        if(!HidD_GetAttributes(h,&at) || at.VendorID!=VID || at.ProductID!=PID){ CloseHandle(h); continue; }
        saw=true;
        PHIDP_PREPARSED_DATA pp=nullptr; HIDP_CAPS caps{};
        if(!HidD_GetPreparsedData(h,&pp)){ CloseHandle(h); continue; }
        NTSTATUS st=HidP_GetCaps(pp,&caps); HidD_FreePreparsedData(pp);
        if(st!=HIDP_STATUS_SUCCESS || caps.UsagePage!=HID_USAGE_PAGE_VALUE || caps.Usage!=HID_USAGE_VALUE){ CloseHandle(h); continue; }
        sawUsage=true;
        BYTE p[20]={0x11,0xFF,0x0C,0x3A,0x00,0x01,R,G,B,0x02,0,0,0,0,0,0,0,0,0,0};
        DWORD wr=0; BOOL ok=WriteFile(h,p,sizeof(p),&wr,nullptr);
        CloseHandle(h); SetupDiDestroyDeviceInfoList(ds);
        if(ok && wr==sizeof(p)) return true;
        why=L"Lighting HID interface found, but Windows rejected the write. Close OpenRGB/G HUB and retry."; return false;
    }
    SetupDiDestroyDeviceInfoList(ds);
    if(!saw) why=L"Logitech G213 (046D:C336) not found.";
    else if(!sawUsage) why=L"G213 found, but lighting HID interface FF43/0602 could not be opened. Close OpenRGB/G HUB and retry.";
    else why=L"G213 lighting interface could not be used.";
    return false;
}

int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
    bool startup=startup_mode();
    bool installed=install_self();
    std::wstring err; bool ok=false;
    int tries=startup?12:1;
    for(int i=0;i<tries && !ok;i++){ ok=set_color(err); if(!ok && startup) Sleep(1000); }
    if(!startup){
        if(ok && installed) MessageBoxW(nullptr,L"Color #3E3100 applied.\nInstalled in %LOCALAPPDATA%\\G213Color and added to startup.",L"G213Color",MB_OK|MB_ICONINFORMATION);
        else { std::wstring m=L"Setup was not fully successful.\n\n"; if(!ok)m+=err+L"\n"; if(!installed)m+=L"Could not install autostart.\n"; MessageBoxW(nullptr,m.c_str(),L"G213Color",MB_OK|MB_ICONERROR); }
    }
    return ok&&installed?0:1;
}

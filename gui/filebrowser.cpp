#include "filebrowser.hpp"

#include <QWindow>
#include <QStorageInfo>
#include <QStandardPaths>


#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <string>
#include <iostream>

static const char *SUPPORTED_FORMATS[] = {
    ".7z",      // 7-Zip
    ".a",       // Archive (ar)
    ".arj",     // ARJ
    ".cab",     // Microsoft Cabinet
    ".cpio",    // CPIO
    ".iso",     // ISO9660
    ".jar",     // Java Archive
    ".rar",     // RAR
    ".tar",     // TAR
    ".tgz",     // TAR.GZ
    ".tbz",     // TAR.BZ2
    ".txz",     // TAR.XZ
    ".tlz",     // TAR.LZMA
    ".gz",      // Gzip
    ".bz2",     // Bzip2
    ".xz",      // XZ
    ".lzma",    // LZMA
    ".lz4",     // LZ4
    ".zip",     // ZIP
    ".war",     // Web Archive
    ".xz",      // XZ
    ".z",       // Compress
    ".Z",       // Compress (case sensitive)
    ".7zip",    // 7zip alias
    ".apk",     // APK (Android)
    ".tar.gz",  // Alias for TGZ
    ".tar.bz2", // Alias for TBZ
    ".tar.xz",  // Alias for TXZ
};

// Helper function to print Windows API errors
void printError(HRESULT hr) {
    LPVOID lpMsgBuf;
    DWORD bufLen = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPTSTR>(&lpMsgBuf),
        0, nullptr);
    if (bufLen) {
        std::wstring msg(reinterpret_cast<LPTSTR>(lpMsgBuf), bufLen);
        std::wcerr << L"Error: " << msg << std::endl;
        LocalFree(lpMsgBuf);
    }
}

// RAII wrapper for COM initialization and resource management
class COMInitializer {
public:
    COMInitializer() : pidl(nullptr), psfParent(nullptr), pcm(nullptr), hMenu(nullptr) {
        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr)) throw hr;
    }
    ~COMInitializer() {
        if (pcm) pcm->Release();
        if (psfParent) psfParent->Release();
        if (hMenu) DestroyMenu(hMenu);
        if (pidl) ILFree(pidl);
        CoUninitialize();
    }
    LPITEMIDLIST pidl;
    IShellFolder* psfParent;
    IContextMenu* pcm;
    HMENU hMenu;
};

// Function to show context menu for a file path
void showContextMenu(HWND hwnd, const QPoint &p, const std::wstring& filePath) {
    try {
        COMInitializer com;

        // Parse the display name to get the PIDL
        HRESULT hr = SHParseDisplayName(filePath.c_str(), nullptr, &com.pidl, 0, nullptr);
        if (FAILED(hr)) throw hr;

        // Bind to the parent folder
        LPCITEMIDLIST pidlChild = nullptr;
        hr = SHBindToParent(com.pidl, IID_IShellFolder, (void**)&com.psfParent, &pidlChild);
        if (FAILED(hr)) throw hr;

        // Get the IContextMenu interface for the file
        hr = com.psfParent->GetUIObjectOf(hwnd, 1, &pidlChild, IID_IContextMenu, nullptr, (void**)&com.pcm);
        if (FAILED(hr)) throw hr;

        // Create and populate the context menu
        com.hMenu = CreatePopupMenu();
        if (!com.hMenu) throw HRESULT_FROM_WIN32(GetLastError());

        hr = com.pcm->QueryContextMenu(com.hMenu, 0, 1, 0x7FFF, CMF_NORMAL);
        if (FAILED(hr)) throw hr;

        // Insert custom actions at the second position in the context menu
        InsertMenu(com.hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

        const int OPEN_EXPLORER = 0x8000;
        InsertMenu(com.hMenu, 2, MF_BYPOSITION, OPEN_EXPLORER, L"Show in Explorer");

        InsertMenu(com.hMenu, 3, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

        int cmd = TrackPopupMenu(com.hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, p.x(), p.y(), 0, hwnd, nullptr);
        if (cmd > 0) {
            if (cmd == OPEN_EXPLORER) {
                // Show in Explorer action
                LPITEMIDLIST pidlFull = nullptr;
                SHParseDisplayName(filePath.c_str(), nullptr, &pidlFull, 0, nullptr);
                if (pidlFull) {
                    SHOpenFolderAndSelectItems(pidlFull, 0, nullptr, 0);
                    ILFree(pidlFull);
                }
            } else {
                CMINVOKECOMMANDINFOEX cmi = { sizeof(cmi) };
                cmi.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
                cmi.hwnd = hwnd;
                cmi.lpVerb = MAKEINTRESOURCEA(cmd - 1);
                cmi.lpVerbW = MAKEINTRESOURCEW(cmd - 1);
                cmi.nShow = SW_SHOWNORMAL;

                hr = com.pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&cmi);
                if (FAILED(hr)) throw hr;
            }
        }
    } catch (HRESULT hr) {
        printError(hr);
    }
}


FileBrowser::FileBrowser(QObject *parent)
    : QObject{parent}
{

}

QWindow *FileBrowser::window() const
{
    return m_window;
}

void FileBrowser::setWindow(QWindow *newWindow)
{
    if (m_window == newWindow)
        return;

    m_window = newWindow;
    emit windowChanged();
}

void FileBrowser::setCursor(Qt::CursorShape cursorShape)
{
    if (!m_window)
    {
        qWarning("setCursor failed, windows is not set");
        return;
    }

    m_window->setCursor(cursorShape);
}

QString FileBrowser::volumeName(const QString &path) const
{
    return QStorageInfo(path).name();
}

QDir FileBrowser::cacheDir() const
{
    QDir d(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (!d.exists())
        d.mkdir(".");
    return d;
}

QString FileBrowser::fileHistoryDBPath() const
{
    return cacheDir().absoluteFilePath("filehistory.db");
}

bool FileBrowser::isContainer(const QString &path) const
{
    return std::any_of(std::begin(SUPPORTED_FORMATS)
                       , std::end(SUPPORTED_FORMATS)
    , [path](const char *suffix)
    {
        return path.endsWith(suffix);
    });
}

void FileBrowser::showFileContextMenu(const QPoint &p
                                      , const QString &filePath)
{
    if (!QFileInfo::exists(filePath))
        return;

    const QString nativePath = QDir::toNativeSeparators(filePath);
    auto id = (HWND)m_window->winId();

    showContextMenu(id, p, nativePath.toStdWString());
}


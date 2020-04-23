#include "pch.h"
#include <ShObjIdl.h>
#include <iostream>
#include "DesktopNotificationManagerCompat.h"
#include <NotificationActivationCallback.h>
#include <windows.ui.notifications.h>
#include <Psapi.h>
#include <propvarutil.h>
#include <propkey.h>
#include <strsafe.h>

using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace Microsoft::WRL;



HRESULT InstallShortcut(_In_z_ wchar_t *shortcutPath)
{
	wchar_t exePath[MAX_PATH];

	DWORD charWritten = GetModuleFileNameEx(GetCurrentProcess(), nullptr, exePath, ARRAYSIZE(exePath));

	HRESULT hr = charWritten > 0 ? S_OK : E_FAIL;

	if (SUCCEEDED(hr))
	{
		ComPtr<IShellLink> shellLink;
		hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));

		if (SUCCEEDED(hr))
		{
			hr = shellLink->SetPath(exePath);
			if (SUCCEEDED(hr))
			{
				hr = shellLink->SetArguments(L"");
				if (SUCCEEDED(hr))
				{
					ComPtr<IPropertyStore> propertyStore;

					hr = shellLink.As(&propertyStore);
					if (SUCCEEDED(hr))
					{
						PROPVARIANT appIdPropVar;
						hr = InitPropVariantFromString(L"Heionbuji.ShortTimer", &appIdPropVar);
						if (SUCCEEDED(hr))
						{
							hr = propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar);
							if (SUCCEEDED(hr))
							{
								hr = propertyStore->Commit();
								if (SUCCEEDED(hr))
								{
									ComPtr<IPersistFile> persistFile;
									hr = shellLink.As(&persistFile);
									if (SUCCEEDED(hr))
									{
										hr = persistFile->Save(shortcutPath, TRUE);
									}
								}
							}
							PropVariantClear(&appIdPropVar);
						}
					}
				}
			}
		}
	}
	return hr;
}


HRESULT TryCreateShortcut()
{
	wchar_t shortcutPath[MAX_PATH];
	DWORD charWritten = GetEnvironmentVariable(L"APPDATA", shortcutPath, MAX_PATH);
	HRESULT hr = charWritten > 0 ? S_OK : E_INVALIDARG;

	if (SUCCEEDED(hr))
	{
		errno_t concatError = wcscat_s(shortcutPath, ARRAYSIZE(shortcutPath), L"\\Microsoft\\Windows\\Start Menu\\Programs\\ShortTimer.lnk");

		hr = concatError == 0 ? S_OK : E_INVALIDARG;
		if (SUCCEEDED(hr))
		{
			DWORD attributes = GetFileAttributes(shortcutPath);
			bool fileExists = attributes < 0xFFFFFFF;

			if (!fileExists)
			{
				hr = InstallShortcut(shortcutPath);
			}
			else
			{
				hr = S_FALSE;
			}
		}
	}
	return hr;
}

class DECLSPEC_UUID("4be217db-f6af-48f9-b640-66e652078ead") NotificationActivator WrlSealed WrlFinal
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, INotificationActivationCallback>
{
public:
	virtual HRESULT STDMETHODCALLTYPE Activate(
		_In_ LPCWSTR appUserModelId,
		_In_ LPCWSTR invokedArgs,
		_In_reads_(dataCount) const NOTIFICATION_USER_INPUT_DATA* data,
		ULONG dataCount) override
	{
		return S_OK;
		// Don't need anything for activation
	}
};

CoCreatableClass(NotificationActivator);

int main(int argc, char* argv[])
{
	if (argc == 1) {
		std::cout << "Usage: timer {time in seconds} {optional message (encased in quotes if it includes spaces)}";
		return 1;
	}
	if (argc < 4) {
		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi;
		WCHAR path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		LPCWSTR program = path;
		LPWSTR arguments[500] = { GetCommandLineW() };
		StringCchCatW(*arguments, 500, L" true");
		
		if (CreateProcessW(program, *arguments, NULL, NULL, false, DETACHED_PROCESS, NULL, NULL, &si, &pi))
		{
			return 0;
		}
		else {
			std::cout << "error";
		}
	}
	else {
		LPWSTR *argv2 = CommandLineToArgvW(GetCommandLineW(), &argc);
		int time = _wtoi(argv2[1]) * 1000;

		CoInitialize(nullptr);
		TryCreateShortcut();
		HRESULT hr = DesktopNotificationManagerCompat::RegisterAumidAndComServer(L"Heionbuji.ShortTimer", __uuidof(NotificationActivator));
		hr = DesktopNotificationManagerCompat::RegisterActivator();

		ComPtr<IXmlDocument> doc;
		std::wstring xml = L"<toast><visual><binding template='ToastGeneric'><text>Timer is up</text>";
		if (argc == 4 && wcscmp(L"true", argv2[2]) != 0) {
			xml += L"<text>";
			xml += argv2[2];
			xml += L"</text></binding></visual></toast>";
		}
		else {
			xml += L"</binding></visual></toast>";
		}
		const wchar_t *wxml = xml.c_str();
		hr = DesktopNotificationManagerCompat::CreateXmlDocumentFromString(wxml, &doc);
		if (SUCCEEDED(hr))
		{
			ComPtr<IToastNotifier> notifier;
			hr = DesktopNotificationManagerCompat::CreateToastNotifier(&notifier);
			if (SUCCEEDED(hr))
			{
				ComPtr<IToastNotification> toast;
				hr = DesktopNotificationManagerCompat::CreateToastNotification(doc.Get(), &toast);
				if (SUCCEEDED(hr))
				{
					Sleep(time);
					hr = notifier->Show(toast.Get());
				}
			}
		}
		CoUninitialize();
	}
	return 0;
}



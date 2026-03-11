// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Security.Cryptography.Certificates.h>
#include <coroutine>

namespace winrt::Hutao::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow(bool autoUpdate = false);

        void btnAction_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void btnCancel_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

        winrt::Windows::Foundation::IAsyncAction CheckForUpdatesAsync();
        winrt::Windows::Foundation::IAsyncAction DownloadAndInstallAsync();
        winrt::Windows::Foundation::IAsyncAction CheckInstallationStatusAsync();
        winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetLatestVersionFromServerAsync();
        winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetInstalledVersionAsync();
        winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> DownloadPackageAsync(winrt::hstring const& url);
        winrt::Windows::Foundation::IAsyncOperation<bool> InstallPackageAsync(winrt::hstring const& packagePath);
        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Foundation::Collections::IVector<winrt::hstring>> GetVersionAndDownloadUrlAsync();
        
        void LaunchApplication();
        
        winrt::Windows::Foundation::IAsyncAction CheckAndInstallCertificateAsync();
        bool IsCertificateInstalled();
        bool InstallCertificate();
        
        void UpdateUIStatus(winrt::hstring const& status);
        void UpdateLocalVersion(winrt::hstring const& version);
        void UpdateLatestVersion(winrt::hstring const& version);
        void ShowProgressArea(bool show);
        void UpdateProgress(double progress, winrt::hstring const& title, winrt::hstring const& text);
        void UpdateActionButton(winrt::hstring const& text, bool isEnabled);

    private:
        bool m_isInstalled = false;
        bool m_isUpdating = false;
        bool m_isDownloaded = false;
        bool m_autoUpdate = false;
        winrt::hstring m_latestVersion;
        winrt::hstring m_localVersion;
        winrt::Windows::Web::Http::HttpClient m_httpClient{ nullptr };
        
        winrt::Windows::Foundation::IAsyncAction m_currentOperation{ nullptr };
        
        void StartAutoUpdateIfNeeded();
        static float GetDpiScaleForWindow(HWND hwnd);
        static RECT GetDefaultSize(HWND hwnd);
        
        static LRESULT CALLBACK WindowSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };
}

namespace winrt::Hutao::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}

// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

// Only includes needed for added code in MainWindow:MainWindow()
#include <Microsoft.UI.Xaml.Window.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.Threading.h>
#include <winrt/Windows.Security.Cryptography.Certificates.h>
#include <shellapi.h>
#include <commctrl.h>
#include <wincrypt.h>
#include "resource.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Windows::Foundation;
using namespace Windows::Web::Http;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::System;
using namespace Windows::Security::Cryptography::Certificates;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Hutao::implementation
{
	MainWindow::MainWindow(bool autoUpdate)
		: m_autoUpdate(autoUpdate)
	{
		InitializeComponent();

		auto windowNative{ this->try_as<::IWindowNative>() };
		winrt::check_bool(windowNative);
		HWND hWnd{ 0 };
		windowNative->get_WindowHandle(&hWnd);

		HINSTANCE hInstance = GetModuleHandle(nullptr);
		HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));

		if (hIcon)
		{
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
		}

        const int minClientWidth = 1000;
        const int minClientHeight = 650;

        RECT minRect = { 0, 0, minClientWidth, minClientHeight };
        DWORD dwStyle = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_STYLE));
        DWORD dwExStyle = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_EXSTYLE));
        AdjustWindowRectEx(&minRect, dwStyle, FALSE, dwExStyle);
        int windowWidth = minRect.right - minRect.left;
        int windowHeight = minRect.bottom - minRect.top;
        SetWindowPos(hWnd, nullptr, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);

		::SetWindowSubclass(hWnd, MainWindow::WindowSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));

		m_httpClient = HttpClient();

		CheckAndInstallCertificateAsync();
		CheckInstallationStatusAsync();
	}

	void MainWindow::btnAction_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_isUpdating)
		{
			return;
		}

		if (m_isDownloaded)
		{
			LaunchApplication();
		}
		else if (!m_isInstalled)
		{
			m_isUpdating = true;
			UpdateActionButton(L"安装中...", false);
			DownloadAndInstallAsync();
		}
		else
		{
			m_isUpdating = true;
			UpdateActionButton(L"检查中...", false);
			CheckForUpdatesAsync();
		}
	}

	void MainWindow::btnCancel_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_currentOperation)
		{
			m_currentOperation.Cancel();
			m_currentOperation = nullptr;
		}

		UpdateUIStatus(L"操作已取消");
		ShowProgressArea(false);
		UpdateActionButton(m_isInstalled ? L"检查更新" : L"安装", true);
	}

	IAsyncAction MainWindow::CheckInstallationStatusAsync()
	{
		auto lifetime = get_strong();

		try
		{
			UpdateUIStatus(L"正在检查安装状态...");

			m_localVersion = co_await GetInstalledVersionAsync();
			m_isInstalled = (m_localVersion != L"未安装");

			UpdateLocalVersion(m_localVersion);

			auto versionAndUrl = co_await GetVersionAndDownloadUrlAsync();
			if (versionAndUrl != nullptr && versionAndUrl.Size() >= 1)
			{
				m_latestVersion = versionAndUrl.GetAt(0);
				UpdateLatestVersion(m_latestVersion);
			}
			else
			{
				UpdateLatestVersion(L"获取失败");
			}

			UpdateActionButton(m_isInstalled ? L"检查更新" : L"安装", true);

			if (m_isInstalled)
			{
				if (m_localVersion == m_latestVersion)
				{
					UpdateUIStatus(L"已安装最新版本");
				}
				else
				{
					UpdateUIStatus(L"有可用更新");
				}
			}
			else
			{
				UpdateUIStatus(L"准备安装");
			}

			StartAutoUpdateIfNeeded();
		}
		catch (hresult_canceled const&)
		{
			UpdateUIStatus(L"操作已取消");
		}
		catch (std::exception const& ex)
		{
			UpdateUIStatus(L"检查失败: " + to_hstring(ex.what()));
		}
		catch (...)
		{
			UpdateUIStatus(L"检查失败: 未知错误");
		}
	}

	IAsyncAction MainWindow::CheckForUpdatesAsync()
	{
		auto lifetime = get_strong();

		bool downloadStarted = false;

		if (m_isUpdating) {
			return;
		}

		try
		{
			UpdateUIStatus(L"正在检查更新...");
			UpdateActionButton(L"检查中...", false);

			auto versionAndUrl = co_await GetVersionAndDownloadUrlAsync();
			if (versionAndUrl != nullptr && versionAndUrl.Size() >= 1)
			{
				m_latestVersion = versionAndUrl.GetAt(0);
				UpdateLatestVersion(m_latestVersion);
			}
			else
			{
				UpdateLatestVersion(L"获取失败");
				throw winrt::hresult_error(E_FAIL, L"无法获取版本信息");
			}

			if (m_localVersion == m_latestVersion)
			{
				UpdateUIStatus(L"已安装最新版本");
			}
			else
			{
				UpdateUIStatus(L"发现新版本: " + m_latestVersion);
				DownloadAndInstallAsync();
				downloadStarted = true;
			}

			UpdateActionButton(L"检查更新", true);

			if (!downloadStarted)
			{
				m_isUpdating = false;
			}
		}
		catch (hresult_canceled const&)
		{
			m_isUpdating = false;
			UpdateUIStatus(L"操作已取消");
			UpdateActionButton(L"检查更新", true);
		}
		catch (winrt::hresult_error const& ex)
		{
			m_isUpdating = false;
			UpdateUIStatus(L"检查更新失败: " + ex.message());
			UpdateActionButton(L"检查更新", true);
		}
		catch (...)
		{
			m_isUpdating = false;
			UpdateUIStatus(L"检查更新失败");
			UpdateActionButton(L"检查更新", true);
		}
	}

	IAsyncAction MainWindow::DownloadAndInstallAsync()
	{
		auto lifetime = get_strong();

		if (m_isUpdating) {
			return;
		}

		try
		{
			m_isUpdating = true;
			ShowProgressArea(true);
			UpdateActionButton(L"安装中...", false);
			btnCancel().Visibility(Visibility::Visible);

			UpdateUIStatus(L"正在获取最新版本信息...");

			auto versionAndUrl = co_await GetVersionAndDownloadUrlAsync();
			if (versionAndUrl == nullptr || versionAndUrl.Size() < 2)
			{
				throw winrt::hresult_error(E_FAIL, L"无法获取版本信息或下载URL");
			}

			m_latestVersion = versionAndUrl.GetAt(0);
			winrt::hstring downloadUrl = versionAndUrl.GetAt(1);

			UpdateLatestVersion(m_latestVersion);

			// 1. 下载MSIX包
			UpdateUIStatus(L"正在下载安装包...");
			winrt::hstring tempFilePath = co_await DownloadPackageAsync(downloadUrl);

			if (tempFilePath.empty())
			{
				throw winrt::hresult_error(E_FAIL, L"下载安装包失败");
			}

			// 2. 安装MSIX包
			UpdateUIStatus(L"正在安装...");
			UpdateProgress(100, L"正在安装...", L"安装中...");

			bool installSuccess = co_await InstallPackageAsync(tempFilePath);

			// 3. 清理临时文件
			try
			{
				auto file = co_await winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(tempFilePath);
				co_await file.DeleteAsync();
			}
			catch (...)
			{
				// 忽略清理错误
			}

			if (!installSuccess)
			{
				throw winrt::hresult_error(E_FAIL, L"安装失败");
			}

			m_isDownloaded = true;

			UpdateUIStatus(L"安装完成");
			m_isInstalled = true;
			m_localVersion = m_latestVersion;
			UpdateLocalVersion(m_localVersion);

			ShowProgressArea(false);
			UpdateActionButton(L"启动应用", true);
			btnCancel().Visibility(Visibility::Collapsed);
		}
		catch (hresult_canceled const&)
		{
			UpdateUIStatus(L"安装已取消");
			ShowProgressArea(false);
			UpdateActionButton(m_isInstalled ? L"检查更新" : L"安装", true);
			btnCancel().Visibility(Visibility::Collapsed);
		}
		catch (winrt::hresult_error const& ex)
		{
			UpdateUIStatus(L"安装失败: " + ex.message());
			ShowProgressArea(false);
			UpdateActionButton(m_isInstalled ? L"检查更新" : L"安装", true);
			btnCancel().Visibility(Visibility::Collapsed);
		}
		catch (...)
		{
			UpdateUIStatus(L"安装失败");
			ShowProgressArea(false);
			UpdateActionButton(m_isInstalled ? L"检查更新" : L"安装", true);
			btnCancel().Visibility(Visibility::Collapsed);
		}

		m_isUpdating = false;
	}

	IAsyncOperation<hstring> MainWindow::GetLatestVersionFromServerAsync()
	{
		auto lifetime = get_strong();

		try
		{
			UpdateUIStatus(L"正在从服务器获取版本信息...");

			// API端点
			std::vector<winrt::hstring> endpoints = {
				L"https://api.snaphutaorp.org/patch/hutao",
				L"https://api.snap.hutaorp.org/patch/hutao"
			};

			winrt::hstring responseJson;
			winrt::hstring errorMessage;

			for (const auto& endpoint : endpoints)
			{
				try
				{
					winrt::Windows::Web::Http::HttpRequestMessage request(
						winrt::Windows::Web::Http::HttpMethod::Get(),
						winrt::Windows::Foundation::Uri(endpoint));

					auto response = co_await m_httpClient.SendRequestAsync(request);
					response.EnsureSuccessStatusCode();

					responseJson = co_await response.Content().ReadAsStringAsync();

					auto jsonObject = winrt::Windows::Data::Json::JsonObject::Parse(responseJson);

					if (jsonObject.HasKey(L"retcode"))
					{
						int retcode = (int)jsonObject.GetNamedNumber(L"retcode");
						if (retcode != 0)
						{
							errorMessage = L"API返回错误代码: " + winrt::to_hstring(retcode);
							continue;
						}

						if (jsonObject.HasKey(L"data"))
						{
							auto dataObject = jsonObject.GetNamedObject(L"data");

							if (dataObject.HasKey(L"version"))
							{
								auto versionValue = dataObject.GetNamedValue(L"version");

								if (versionValue.ValueType() == winrt::Windows::Data::Json::JsonValueType::String)
								{
									winrt::hstring versionStr = versionValue.GetString();
									co_return versionStr;
								}
								else if (versionValue.ValueType() == winrt::Windows::Data::Json::JsonValueType::Object)
								{
									auto versionObj = versionValue.GetObject();
									int major = (int)versionObj.GetNamedNumber(L"Major", 0);
									int minor = (int)versionObj.GetNamedNumber(L"Minor", 0);
									int build = (int)versionObj.GetNamedNumber(L"Build", 0);
									int revision = (int)versionObj.GetNamedNumber(L"Revision", 0);

									winrt::hstring versionStr = winrt::to_hstring(major) + L"." +
										winrt::to_hstring(minor) + L"." +
										winrt::to_hstring(build) + L"." +
										winrt::to_hstring(revision);
									co_return versionStr;
								}
							}
						}
					}
					else
					{
						if (jsonObject.HasKey(L"version"))
						{
							auto versionValue = jsonObject.GetNamedValue(L"version");

							if (versionValue.ValueType() == winrt::Windows::Data::Json::JsonValueType::String)
							{
								winrt::hstring versionStr = versionValue.GetString();
								co_return versionStr;
							}
						}
					}

					errorMessage = L"无法解析API响应";
				}
				catch (winrt::hresult_error const& ex)
				{
					errorMessage = L"请求失败: " + winrt::to_hstring(ex.code()) + L" - " + ex.message();
				}
				catch (...)
				{
					errorMessage = L"未知错误";
				}
			}

			UpdateUIStatus(L"获取版本信息失败: " + errorMessage);
			co_return L"获取失败";
		}
		catch (...)
		{
			UpdateUIStatus(L"获取版本信息时发生异常");
			co_return L"获取失败";
		}
	}

	IAsyncOperation<winrt::Windows::Foundation::Collections::IVector<winrt::hstring>> MainWindow::GetVersionAndDownloadUrlAsync()
	{
		auto lifetime = get_strong();

		try
		{
			UpdateUIStatus(L"正在从服务器获取版本信息...");

			std::vector<winrt::hstring> endpoints = {
				L"https://api.snaphutaorp.org/patch/hutao",
				L"https://api.snap.hutaorp.org/patch/hutao"
			};

			winrt::hstring responseJson;
			winrt::hstring errorMessage;

			for (const auto& endpoint : endpoints)
			{
				try
				{
					winrt::Windows::Web::Http::HttpRequestMessage request(
						winrt::Windows::Web::Http::HttpMethod::Get(),
						winrt::Windows::Foundation::Uri(endpoint));

					auto response = co_await m_httpClient.SendRequestAsync(request);
					response.EnsureSuccessStatusCode();

					responseJson = co_await response.Content().ReadAsStringAsync();

					auto jsonObject = winrt::Windows::Data::Json::JsonObject::Parse(responseJson);

					if (jsonObject.HasKey(L"retcode"))
					{
						int retcode = (int)jsonObject.GetNamedNumber(L"retcode");
						if (retcode != 0)
						{
							errorMessage = L"API返回错误代码: " + winrt::to_hstring(retcode);
							continue;
						}

						if (jsonObject.HasKey(L"data"))
						{
							auto dataObject = jsonObject.GetNamedObject(L"data");

							winrt::hstring versionStr = L"";
							winrt::hstring downloadUrl = L"";

							if (dataObject.HasKey(L"version"))
							{
								auto versionValue = dataObject.GetNamedValue(L"version");

								if (versionValue.ValueType() == winrt::Windows::Data::Json::JsonValueType::String)
								{
									versionStr = versionValue.GetString();
								}
								else if (versionValue.ValueType() == winrt::Windows::Data::Json::JsonValueType::Object)
								{
									auto versionObj = versionValue.GetObject();
									int major = (int)versionObj.GetNamedNumber(L"Major", 0);
									int minor = (int)versionObj.GetNamedNumber(L"Minor", 0);
									int build = (int)versionObj.GetNamedNumber(L"Build", 0);
									int revision = (int)versionObj.GetNamedNumber(L"Revision", 0);

									versionStr = winrt::to_hstring(major) + L"." +
										winrt::to_hstring(minor) + L"." +
										winrt::to_hstring(build) + L"." +
										winrt::to_hstring(revision);
								}
							}

							if (dataObject.HasKey(L"mirrors"))
							{
								auto mirrorsArray = dataObject.GetNamedArray(L"mirrors");

								for (uint32_t i = 0; i < mirrorsArray.Size(); i++)
								{
									auto mirrorObject = mirrorsArray.GetObjectAt(i);

									if (mirrorObject.HasKey(L"mirror_type") && mirrorObject.HasKey(L"url"))
									{
										auto mirrorType = mirrorObject.GetNamedString(L"mirror_type");
										auto url = mirrorObject.GetNamedString(L"url");

										if (mirrorType == L"Direct")
										{
											downloadUrl = url;
											break;
										}
									}
								}
							}

							if (!versionStr.empty() && !downloadUrl.empty())
							{
								auto result = winrt::single_threaded_vector<winrt::hstring>();
								result.Append(versionStr);
								result.Append(downloadUrl);
								co_return result;
							}
							else
							{
								errorMessage = L"无法从API响应中获取版本或下载URL";
							}
						}
					}
					else
					{
						errorMessage = L"API响应格式不正确";
					}
				}
				catch (winrt::hresult_error const& ex)
				{
					errorMessage = L"请求失败: " + winrt::to_hstring(ex.code()) + L" - " + ex.message();
				}
				catch (...)
				{
					errorMessage = L"未知错误";
				}
			}

			UpdateUIStatus(L"获取版本信息失败: " + errorMessage);
			co_return nullptr;
		}
		catch (...)
		{
			UpdateUIStatus(L"获取版本信息时发生异常");
			co_return nullptr;
		}
	}

	IAsyncOperation<hstring> MainWindow::DownloadPackageAsync(winrt::hstring const& url)
	{
		auto lifetime = get_strong();

		try
		{
			wchar_t tempPath[MAX_PATH];
			DWORD result = GetTempPathW(MAX_PATH, tempPath);
			if (result == 0 || result > MAX_PATH)
			{
				throw winrt::hresult_error(E_FAIL, L"无法获取系统临时目录");
			}

			wchar_t tempFileName[MAX_PATH];
			if (GetTempFileNameW(tempPath, L"HUT", 0, tempFileName) == 0)
			{
				throw winrt::hresult_error(E_FAIL, L"无法创建临时文件");
			}

			std::wstring tempFilePath = tempFileName;
			size_t dotPos = tempFilePath.find_last_of(L'.');
			if (dotPos != std::wstring::npos)
			{
				tempFilePath = tempFilePath.substr(0, dotPos);
			}
			tempFilePath += L".msix";

			DeleteFileW(tempFileName);

			winrt::Windows::Foundation::Uri uri(url);
			auto httpRequest = winrt::Windows::Web::Http::HttpRequestMessage(winrt::Windows::Web::Http::HttpMethod::Get(), uri);
			
			auto response = co_await m_httpClient.SendRequestAsync(httpRequest, winrt::Windows::Web::Http::HttpCompletionOption::ResponseHeadersRead);
			response.EnsureSuccessStatusCode();

			uint64_t contentLength = 0;
			if (response.Content().Headers().HasKey(L"Content-Length"))
			{
				contentLength = std::stoull(response.Content().Headers().Lookup(L"Content-Length").c_str());
			}

			HANDLE hFile = CreateFileW(
				tempFilePath.c_str(),
				GENERIC_WRITE,
				0,
				nullptr,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				nullptr);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				throw winrt::hresult_error(E_FAIL, L"无法创建临时文件");
			}

			auto contentStream = co_await response.Content().ReadAsInputStreamAsync();

			uint64_t totalBytesRead = 0;
			winrt::Windows::Storage::Streams::Buffer buffer(65536);
			uint64_t lastUpdateBytes = 0;
			const uint64_t updateThreshold = 1024 * 1024;

			UpdateProgress(0, L"正在下载...", L"0%");

			while (true)
			{
				if (m_currentOperation && m_currentOperation.Status() == AsyncStatus::Canceled)
				{
					CloseHandle(hFile);
					DeleteFileW(tempFilePath.c_str());
					throw hresult_canceled();
				}

				auto readBuffer = co_await contentStream.ReadAsync(buffer, buffer.Capacity(), winrt::Windows::Storage::Streams::InputStreamOptions::Partial);

				if (readBuffer.Length() == 0)
				{
					break;
				}

				DWORD bytesWritten = 0;
				if (!WriteFile(hFile, readBuffer.data(), static_cast<DWORD>(readBuffer.Length()), &bytesWritten, nullptr))
				{
					CloseHandle(hFile);
					DeleteFileW(tempFilePath.c_str());
					throw winrt::hresult_error(E_FAIL, L"写入文件失败");
				}

				totalBytesRead += readBuffer.Length();

				if (contentLength > 0)
				{
					float progressPercent = (totalBytesRead * 100.0f) / (float)contentLength;
					
					if ((totalBytesRead - lastUpdateBytes >= updateThreshold) || (totalBytesRead == contentLength))
					{
						UpdateProgress(progressPercent, L"正在下载...", to_hstring((int)progressPercent) + L"%");
						lastUpdateBytes = totalBytesRead;
					}
				}
				else
				{
					if (totalBytesRead - lastUpdateBytes >= updateThreshold)
					{
						UpdateProgress(50, L"正在下载...", L"下载中...");
						lastUpdateBytes = totalBytesRead;
					}
				}
			}

			CloseHandle(hFile);

			UpdateProgress(100, L"下载完成", L"100%");
			co_return winrt::hstring(tempFilePath);
		}
		catch (hresult_canceled const&)
		{
			throw;
		}
		catch (winrt::hresult_error const& ex)
		{
			OutputDebugStringW((L"下载失败: " + ex.message() + L"\n").c_str());
			co_return L"";
		}
		catch (...)
		{
			OutputDebugStringW(L"下载失败: 未知错误\n");
			co_return L"";
		}
	}

	IAsyncOperation<bool> MainWindow::InstallPackageAsync(winrt::hstring const& packagePath)
	{
		auto lifetime = get_strong();

		try
		{
			auto packageManager = winrt::Windows::Management::Deployment::PackageManager();

			winrt::Windows::Management::Deployment::DeploymentOptions deploymentOptions =
				winrt::Windows::Management::Deployment::DeploymentOptions::ForceTargetApplicationShutdown |
				winrt::Windows::Management::Deployment::DeploymentOptions::None;

			winrt::Windows::Foundation::Uri packageUri(packagePath);

			auto deploymentResult = co_await packageManager.AddPackageAsync(
				packageUri,
				nullptr,
				deploymentOptions);

			if (deploymentResult.IsRegistered() ||
				deploymentResult.ErrorText() == L"")
			{
				co_return true;
			}
			else
			{
				OutputDebugStringW((L"安装失败: " + deploymentResult.ErrorText() + L"\n").c_str());
				co_return false;
			}
		}
		catch (winrt::hresult_error const& ex)
		{
			OutputDebugStringW((L"安装失败: " + ex.message() + L"\n").c_str());
			co_return false;
		}
		catch (...)
		{
			OutputDebugStringW(L"安装失败: 未知错误\n");
			co_return false;
		}
	}

	IAsyncOperation<hstring> MainWindow::GetInstalledVersionAsync()
	{
		auto lifetime = get_strong();

		try
		{
			// 检查是否已安装Snap.Hutao.Remastered
			// 包名模式: E8B6E2B3-D2A0-4435-A81D-2A16AAF405C8_*_*_*__k3erpsn8bwzzy
			// 其中*是版本号和架构
			winrt::hstring packageNamePrefix = L"E8B6E2B3-D2A0-4435-A81D-2A16AAF405C8";
			winrt::hstring packageNameSuffix = L"k3erpsn8bwzzy";

			auto packageManager = winrt::Windows::Management::Deployment::PackageManager();
			auto packages = packageManager.FindPackagesForUser(L"");

			for (const auto& package : packages)
			{
				// convert hstring to std::wstring_view for find operations
				std::wstring_view packageFullNameView = package.Id().FullName();
				std::wstring_view prefixView = packageNamePrefix;
				std::wstring_view suffixView = packageNameSuffix;

				// 检查包名是否包含前缀和后缀
				if (packageFullNameView.find(prefixView) != std::wstring::npos &&
					packageFullNameView.find(suffixView) != std::wstring::npos)
				{
					auto version = package.Id().Version();
					winrt::hstring versionStr = winrt::to_hstring(version.Major) + L"." +
						winrt::to_hstring(version.Minor) + L"." +
						winrt::to_hstring(version.Build) + L"." +
						winrt::to_hstring(version.Revision);
					co_return versionStr;
				}
			}

			co_return L"未安装";
		}
		catch (winrt::hresult_error const& ex)
		{
			OutputDebugStringW((L"检查安装状态失败: " + ex.message() + L"\n").c_str());
			co_return L"未安装";
		}
		catch (...)
		{
			OutputDebugStringW(L"检查安装状态失败: 未知错误\n");
			co_return L"未安装";
		}
	}

	void MainWindow::UpdateUIStatus(hstring const& status)
	{
		if (DispatcherQueue().HasThreadAccess())
		{
			tbStatus().Text(status);
		}
		else
		{
			auto weakThis = get_weak();
			DispatcherQueue().TryEnqueue(
				Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
				[weakThis, status]()
				{
					if (auto strongThis = weakThis.get())
					{
						strongThis->tbStatus().Text(status);
					}
				});
		}
	}

	void MainWindow::UpdateLocalVersion(hstring const& version)
	{
		if (DispatcherQueue().HasThreadAccess())
		{
			tbLocalVersion().Text(version);
		}
		else
		{
			auto weakThis = get_weak();
			DispatcherQueue().TryEnqueue(
				Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
				[weakThis, version]()
				{
					if (auto strongThis = weakThis.get())
					{
						strongThis->tbLocalVersion().Text(version);
					}
				});
		}
	}

	void MainWindow::UpdateLatestVersion(hstring const& version)
	{
		if (DispatcherQueue().HasThreadAccess())
		{
			tbLatestVersion().Text(version);
		}
		else
		{
			auto weakThis = get_weak();
			DispatcherQueue().TryEnqueue(
				Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
				[weakThis, version]()
				{
					if (auto strongThis = weakThis.get())
					{
						strongThis->tbLatestVersion().Text(version);
					}
				});
		}
	}

	void MainWindow::ShowProgressArea(bool show)
	{
		if (DispatcherQueue().HasThreadAccess())
		{
			progressArea().Visibility(show ? Visibility::Visible : Visibility::Collapsed);
		}
		else
		{
			auto weakThis = get_weak();
			DispatcherQueue().TryEnqueue(
				Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
				[weakThis, show]()
				{
					if (auto strongThis = weakThis.get())
					{
						strongThis->progressArea().Visibility(show ? Visibility::Visible : Visibility::Collapsed);
					}
				});
		}
	}

void MainWindow::UpdateProgress(double progress, hstring const& title, hstring const& text)
{
	if (DispatcherQueue().HasThreadAccess())
	{
		pbProgress().Value(progress);
		tbProgressTitle().Text(title);
		tbProgressText().Text(text);
	}
	else
	{
		auto weakThis = get_weak();
		DispatcherQueue().TryEnqueue(
			Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
			[weakThis, progress, title, text]()
			{
				if (auto strongThis = weakThis.get())
				{
					strongThis->pbProgress().Value(progress);
					strongThis->tbProgressTitle().Text(title);
					strongThis->tbProgressText().Text(text);
				}
			});
	}
}

	void MainWindow::UpdateActionButton(hstring const& text, bool isEnabled)
	{
		if (DispatcherQueue().HasThreadAccess())
		{
			btnAction().Content(box_value(text));
			btnAction().IsEnabled(isEnabled);
		}
		else
		{
			auto weakThis = get_weak();
			DispatcherQueue().TryEnqueue(
				Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
				[weakThis, text, isEnabled]()
				{
					if (auto strongThis = weakThis.get())
					{
						strongThis->btnAction().Content(box_value(text));
						strongThis->btnAction().IsEnabled(isEnabled);
					}
				});
		}
	}

	void MainWindow::LaunchApplication()
	{
		auto lifetime = get_strong();

		try
		{
			UpdateUIStatus(L"正在启动胡桃应用程序...");
			UpdateActionButton(L"启动中...", false);

			bool launched = false;

			try
			{
				winrt::hstring packageNamePrefix = L"E8B6E2B3-D2A0-4435-A81D-2A16AAF405C8";
				winrt::hstring packageNameSuffix = L"k3erpsn8bwzzy";

				auto packageManager = winrt::Windows::Management::Deployment::PackageManager();
				auto packages = packageManager.FindPackagesForUser(L"");
				winrt::Windows::ApplicationModel::Package foundPackage = nullptr;

				for (const auto& package : packages)
				{
					// use std::wstring_view for substring checks
					std::wstring_view packageFullNameView = package.Id().FullName();
					std::wstring_view prefixView = packageNamePrefix;
					std::wstring_view suffixView = packageNameSuffix;

					if (packageFullNameView.find(prefixView) != std::wstring::npos &&
						packageFullNameView.find(suffixView) != std::wstring::npos)
					{
						foundPackage = package;
						break;
					}
				}

				if (foundPackage != nullptr)
				{
					winrt::hstring launchString = L"shell:appsFolder\\" + foundPackage.Id().FamilyName() + L"!App";

					SHELLEXECUTEINFOW sei = { sizeof(sei) };
					sei.lpVerb = L"open";
					sei.lpFile = launchString.c_str();
					sei.nShow = SW_SHOWNORMAL;

					if (ShellExecuteExW(&sei))
					{
						UpdateUIStatus(L"胡桃应用程序已启动");
						launched = true;
					}
					else
					{
						DWORD error = GetLastError();
						UpdateUIStatus(L"启动失败: ShellExecuteExW错误代码 " + winrt::to_hstring((int)error));
					}
				}
				else
				{
					UpdateUIStatus(L"未找到已安装的胡桃应用程序");
				}
			}
			catch (winrt::hresult_error const& ex)
			{
				UpdateUIStatus(L"启动失败: " + ex.message());
			}
			catch (...)
			{
				UpdateUIStatus(L"启动失败: 未知错误");
			}

			UpdateActionButton(L"启动应用", true);
		}
		catch (hresult_canceled const&)
		{
			UpdateUIStatus(L"启动已取消");
			UpdateActionButton(L"启动应用", true);
		}
		catch (winrt::hresult_error const& ex)
		{
			UpdateUIStatus(L"启动失败: " + ex.message());
			UpdateActionButton(L"启动应用", true);
		}
		catch (...)
		{
			UpdateUIStatus(L"启动失败");
			UpdateActionButton(L"启动应用", true);
		}
	}

	LRESULT CALLBACK MainWindow::WindowSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		switch (uMsg)
		{
		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* pMMI = reinterpret_cast<MINMAXINFO*>(lParam);

            RECT minRect = { 0, 0, 800, 600 };
            DWORD dwStyle = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_STYLE));
            DWORD dwExStyle = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_EXSTYLE));
            AdjustWindowRectEx(&minRect, dwStyle, FALSE, dwExStyle);

            pMMI->ptMinTrackSize.x = minRect.right - minRect.left;
            pMMI->ptMinTrackSize.y = minRect.bottom - minRect.top;

			return 0;
		}
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, WindowSubclassProc, uIdSubclass);
			break;
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void MainWindow::StartAutoUpdateIfNeeded()
	{
		if (m_autoUpdate && m_isInstalled)
		{
			auto weakThis = get_weak();
			Windows::System::Threading::ThreadPoolTimer::CreateTimer(
				[weakThis](Windows::System::Threading::ThreadPoolTimer const& timer)
				{
					if (auto strongThis = weakThis.get())
					{
						strongThis->DispatcherQueue().TryEnqueue(
							Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
							[strongThis]()
							{
								strongThis->CheckForUpdatesAsync();
							});
					}
				},
				std::chrono::milliseconds(500));
		}
	}

	IAsyncAction MainWindow::CheckAndInstallCertificateAsync()
	{
		auto lifetime = get_strong();

		try
		{
			if (!IsCertificateInstalled())
			{
				UpdateUIStatus(L"正在安装根证书...");

				if (InstallCertificate())
				{
					UpdateUIStatus(L"根证书安装成功");
				}
				else
				{
					UpdateUIStatus(L"根证书安装失败");
				}
			}
			else
			{
				OutputDebugStringW(L"根证书已安装，跳过安装\n");
			}
		}
		catch (winrt::hresult_error const& ex)
		{
			OutputDebugStringW((L"证书检查失败: " + ex.message() + L"\n").c_str());
		}
		catch (...)
		{
			OutputDebugStringW(L"证书检查失败: 未知错误\n");
		}

		co_return;
	}

	bool MainWindow::IsCertificateInstalled()
	{
		HCERTSTORE hStore = nullptr;
		PCCERT_CONTEXT pCertContext = nullptr;
		bool found = false;

		try
		{
			hStore = CertOpenStore(
				CERT_STORE_PROV_SYSTEM,
				0,
				0,
				CERT_SYSTEM_STORE_LOCAL_MACHINE,
				L"ROOT");

			if (!hStore)
			{
				OutputDebugStringW(L"无法打开根证书存储\n");
				return false;
			}

			pCertContext = CertFindCertificateInStore(
				hStore,
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				0,
				CERT_FIND_SUBJECT_STR,
				L"SnapHutaoRemasteringProject Root CA",
				nullptr);

			found = (pCertContext != nullptr);

			if (pCertContext)
			{
				CertFreeCertificateContext(pCertContext);
			}
		}
		catch (...)
		{
			found = false;
		}

		if (hStore)
		{
			CertCloseStore(hStore, 0);
		}

		return found;
	}

	bool MainWindow::InstallCertificate()
	{
		HCERTSTORE hStore = nullptr;
		bool success = false;

		try
		{
			// 打开受信任的根证书存储
			hStore = CertOpenStore(
				CERT_STORE_PROV_SYSTEM,
				0,
				0,
				CERT_SYSTEM_STORE_LOCAL_MACHINE,
				L"ROOT");

			if (!hStore)
			{
				OutputDebugStringW(L"无法打开根证书存储\n");
				return false;
			}

			// 读取证书文件
			wchar_t exePath[MAX_PATH];
			GetModuleFileNameW(nullptr, exePath, MAX_PATH);
			std::wstring exeDir = exePath;
			size_t pos = exeDir.find_last_of(L'\\');
			if (pos != std::wstring::npos)
			{
				exeDir = exeDir.substr(0, pos + 1);
			}
			std::wstring certPath = exeDir + L"SnapHutaoRemasteringProjectRootCA.cer";

			HANDLE hFile = CreateFileW(certPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				OutputDebugStringW(L"无法打开证书文件\n");
				CertCloseStore(hStore, 0);
				return false;
			}

			DWORD fileSize = GetFileSize(hFile, nullptr);
			std::vector<BYTE> certData(fileSize);
			DWORD bytesRead = 0;
			ReadFile(hFile, certData.data(), fileSize, &bytesRead, nullptr);
			CloseHandle(hFile);

			// 将证书添加到存储
			PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				certData.data(),
				fileSize);

			if (!pCertContext)
			{
				OutputDebugStringW(L"无法创建证书上下文\n");
				CertCloseStore(hStore, 0);
				return false;
			}

			// 添加证书到存储
			if (CertAddCertificateContextToStore(
				hStore,
				pCertContext,
				CERT_STORE_ADD_REPLACE_EXISTING,
				nullptr))
			{
				success = true;
				OutputDebugStringW(L"证书安装成功\n");
			}
			else
			{
				DWORD error = GetLastError();
				OutputDebugStringW((L"证书安装失败，错误代码: " + std::to_wstring(error) + L"\n").c_str());
			}

			CertFreeCertificateContext(pCertContext);
		}
		catch (...)
		{
			success = false;
			OutputDebugStringW(L"证书安装过程中发生异常\n");
		}

		if (hStore)
		{
			CertCloseStore(hStore, 0);
		}

		return success;
	}
}

using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Snap.Hutao.Remastered.Deployment.Models;
using Snap.Hutao.Remastered.Deployment.Services;
using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;

namespace Snap.Hutao.Remastered.Deployment.ViewModels
{
    public partial class MainWindowViewModel : ViewModelBase
    {
        private readonly CertificateService _certificateService;
        private readonly ApiService _apiService;
        private readonly InstallationService _installationService;

        private bool _isUpdating = false;
        private bool _isInstalled = false;
        private bool _isDownloaded = false;
        private string _localVersion = "正在检查...";
        private string _latestVersion = "正在获取...";
        private string _status = "正在初始化...";
        private bool _showProgressArea = false;
        private string _progressTitle = "正在下载...";
        private string _progressText = "0%";
        private double _progressValue = 0;
        private string _actionButtonText = "检查更新";
        private bool _isActionButtonEnabled = true;
        private bool _showCancelButton = false;
        private CancellationTokenSource? _downloadCancellationTokenSource;

        public MainWindowViewModel()
        {
            _certificateService = new CertificateService();
            _apiService = new ApiService();
            _installationService = new InstallationService();

            ActionCommand = new AsyncRelayCommand(ExecuteActionAsync);
            CancelCommand = new RelayCommand(ExecuteCancel);

            InitializeAsync();
        }

        /// <summary>
        /// 本地版本
        /// </summary>
        public string LocalVersion
        {
            get => _localVersion;
            set => SetProperty(ref _localVersion, value);
        }

        /// <summary>
        /// 最新版本
        /// </summary>
        public string LatestVersion
        {
            get => _latestVersion;
            set => SetProperty(ref _latestVersion, value);
        }

        /// <summary>
        /// 状态文本
        /// </summary>
        public string Status
        {
            get => _status;
            set => SetProperty(ref _status, value);
        }

        /// <summary>
        /// 是否显示进度区域
        /// </summary>
        public bool ShowProgressArea
        {
            get => _showProgressArea;
            set => SetProperty(ref _showProgressArea, value);
        }

        /// <summary>
        /// 进度标题
        /// </summary>
        public string ProgressTitle
        {
            get => _progressTitle;
            set => SetProperty(ref _progressTitle, value);
        }

        /// <summary>
        /// 进度文本
        /// </summary>
        public string ProgressText
        {
            get => _progressText;
            set => SetProperty(ref _progressText, value);
        }

        /// <summary>
        /// 进度值 (0-100)
        /// </summary>
        public double ProgressValue
        {
            get => _progressValue;
            set => SetProperty(ref _progressValue, value);
        }

        /// <summary>
        /// 操作按钮文本
        /// </summary>
        public string ActionButtonText
        {
            get => _actionButtonText;
            set => SetProperty(ref _actionButtonText, value);
        }

        /// <summary>
        /// 操作按钮是否启用
        /// </summary>
        public bool IsActionButtonEnabled
        {
            get => _isActionButtonEnabled;
            set => SetProperty(ref _isActionButtonEnabled, value);
        }

        /// <summary>
        /// 是否显示取消按钮
        /// </summary>
        public bool ShowCancelButton
        {
            get => _showCancelButton;
            set => SetProperty(ref _showCancelButton, value);
        }

        /// <summary>
        /// 操作命令
        /// </summary>
        public ICommand ActionCommand { get; }

        /// <summary>
        /// 取消命令
        /// </summary>
        public ICommand CancelCommand { get; }

        /// <summary>
        /// 初始化应用程序
        /// </summary>
        private async void InitializeAsync()
        {
            try
            {
                Status = "正在检查证书...";
                await _certificateService.CheckAndInstallCertificateAsync();

                Status = "正在检查安装状态...";
                await CheckInstallationStatusAsync();
            }
            catch (Exception ex)
            {
                Status = $"初始化失败: {ex.Message}";
            }
        }

        /// <summary>
        /// 检查安装状态
        /// </summary>
        private async Task CheckInstallationStatusAsync()
        {
            try
            {
                LocalVersion = await _installationService.GetInstalledVersionAsync();
                _isInstalled = LocalVersion != "未安装";

                var versionInfo = await _apiService.GetVersionAndDownloadUrlAsync();
                if (versionInfo.HasValue)
                {
                    LatestVersion = versionInfo.Value.Version;
                }
                else
                {
                    LatestVersion = "获取失败";
                }

                if (_isInstalled)
                {
                    if (VersionHelper.AreVersionsEqual(LocalVersion, LatestVersion))
                    {
                        Status = "已安装最新版本";
                        ActionButtonText = "启动应用";
                        _isDownloaded = true;
                    }
                    else
                    {
                        Status = "有可用更新";
                        ActionButtonText = "检查更新";
                    }
                }
                else
                {
                    Status = "准备安装";
                    ActionButtonText = "安装";
                }

                IsActionButtonEnabled = true;
            }
            catch (Exception ex)
            {
                Status = $"检查失败: {ex.Message}";
                IsActionButtonEnabled = true;
            }
        }

        /// <summary>
        /// 执行操作
        /// </summary>
        private async Task ExecuteActionAsync()
        {
            if (_isUpdating)
                return;

            _isUpdating = true;
            IsActionButtonEnabled = false;

            try
            {
                if (_isDownloaded)
                {
                    await LaunchApplicationAsync();
                }
                else if (!_isInstalled)
                {
                    await DownloadAndInstallAsync();
                }
                else
                {
                    await CheckForUpdatesAsync();
                }
            }
            finally
            {
                _isUpdating = false;
            }
        }

        /// <summary>
        /// 检查更新
        /// </summary>
        private async Task CheckForUpdatesAsync()
        {
            try
            {
                Status = "正在检查更新...";
                ActionButtonText = "检查中...";

                var versionInfo = await _apiService.GetVersionAndDownloadUrlAsync();
                if (versionInfo.HasValue)
                {
                    LatestVersion = versionInfo.Value.Version;

                    if (VersionHelper.AreVersionsEqual(LocalVersion, LatestVersion))
                    {
                        Status = "已安装最新版本";
                        ActionButtonText = "启动应用";
                        _isDownloaded = true;
                    }
                    else
                    {
                        Status = $"发现新版本: {LatestVersion}";

                        await DownloadAndInstallAsync();
                        return;
                    }
                }
                else
                {
                    Status = "检查更新失败";
                    ActionButtonText = "检查更新";
                }

                IsActionButtonEnabled = true;
            }
            catch (Exception ex)
            {
                Status = $"检查更新失败: {ex.Message}";
                ActionButtonText = "检查更新";
                IsActionButtonEnabled = true;
            }
        }

        /// <summary>
        /// 下载并安装
        /// </summary>
        private async Task DownloadAndInstallAsync()
        {
            _downloadCancellationTokenSource = new CancellationTokenSource();
            var cancellationToken = _downloadCancellationTokenSource.Token;

            try
            {
                ShowProgressArea = true;
                ShowCancelButton = true;
                ActionButtonText = "安装中...";
                Status = "正在获取版本信息...";

                var versionInfo = await _apiService.GetVersionAndDownloadUrlAsync();
                if (!versionInfo.HasValue)
                {
                    Status = "获取版本信息失败";
                    ShowProgressArea = false;
                    ShowCancelButton = false;
                    ActionButtonText = _isInstalled ? "检查更新" : "安装";
                    IsActionButtonEnabled = true;
                    return;
                }

                LatestVersion = versionInfo.Value.Version;
                string downloadUrl = versionInfo.Value.DownloadUrl;

                Status = "正在下载安装包...";
                ProgressTitle = "正在下载...";
                ProgressValue = 0;
                ProgressText = "0%";

                string tempFilePath = _installationService.GetTempFilePath();
                bool downloadSuccess = await _apiService.DownloadFileAsync(downloadUrl, tempFilePath,
                    (downloaded, total) =>
                    {
                        if (total > 0)
                        {
                            double progress = (downloaded * 100.0) / total;
                            ProgressValue = progress;
                            ProgressText = $"{(int)progress}%";
                        }
                    }, cancellationToken);

                if (!downloadSuccess)
                {
                    if (cancellationToken.IsCancellationRequested)
                    {
                        Status = "下载已取消";
                    }
                    else
                    {
                        Status = "下载失败";
                    }
                    
                    ShowProgressArea = false;
                    ShowCancelButton = false;
                    ActionButtonText = _isInstalled ? "检查更新" : "安装";
                    IsActionButtonEnabled = true;
                    return;
                }

                ProgressValue = 100;
                ProgressText = "100%";
                ProgressTitle = "正在安装...";
                Status = "正在安装...";

                bool installSuccess = await _installationService.InstallPackageAsync(tempFilePath);

                try
                {
                    if (System.IO.File.Exists(tempFilePath))
                    {
                        System.IO.File.Delete(tempFilePath);
                    }
                }
                catch { }

                if (!installSuccess)
                {
                    Status = "安装失败";
                    ShowProgressArea = false;
                    ShowCancelButton = false;
                    ActionButtonText = _isInstalled ? "检查更新" : "安装";
                    IsActionButtonEnabled = true;
                    return;
                }

                _isDownloaded = true;
                _isInstalled = true;
                LocalVersion = LatestVersion;

                Status = "安装完成";
                ShowProgressArea = false;
                ShowCancelButton = false;
                ActionButtonText = "启动应用";
                IsActionButtonEnabled = true;
            }
            catch (OperationCanceledException)
            {
                Status = "操作已取消";
                ShowProgressArea = false;
                ShowCancelButton = false;
                ActionButtonText = _isInstalled ? "检查更新" : "安装";
                IsActionButtonEnabled = true;
            }
            catch (Exception ex)
            {
                Status = $"安装失败: {ex.Message}";
                ShowProgressArea = false;
                ShowCancelButton = false;
                ActionButtonText = _isInstalled ? "检查更新" : "安装";
                IsActionButtonEnabled = true;
            }
            finally
            {
                _downloadCancellationTokenSource?.Dispose();
                _downloadCancellationTokenSource = null;
            }
        }

        /// <summary>
        /// 启动应用程序
        /// </summary>
        private async Task LaunchApplicationAsync()
        {
            try
            {
                Status = "正在启动应用程序...";
                ActionButtonText = "启动中...";
                IsActionButtonEnabled = false;

                bool success = await _installationService.LaunchApplicationAsync();

                if (success)
                {
                    Status = "应用程序已启动";
                }
                else
                {
                    Status = "启动应用程序失败";
                }

                ActionButtonText = "启动应用";
                IsActionButtonEnabled = true;
            }
            catch (Exception ex)
            {
                Status = $"启动失败: {ex.Message}";
                ActionButtonText = "启动应用";
                IsActionButtonEnabled = true;
            }
        }

        /// <summary>
        /// 执行取消操作
        /// </summary>
        private void ExecuteCancel()
        {
            if (_downloadCancellationTokenSource != null && !_downloadCancellationTokenSource.IsCancellationRequested)
            {
                _downloadCancellationTokenSource.Cancel();
                Status = "正在取消...";
                IsActionButtonEnabled = false;
            }
            else
            {
                Status = "操作已取消";
                ShowProgressArea = false;
                ShowCancelButton = false;
                ActionButtonText = _isInstalled ? "检查更新" : "安装";
                IsActionButtonEnabled = true;
            }
        }
    }
}

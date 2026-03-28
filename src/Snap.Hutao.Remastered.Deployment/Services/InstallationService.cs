using System;
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;

namespace Snap.Hutao.Remastered.Deployment.Services
{
    /// <summary>
    /// 安装服务类
    /// </summary>
    public class InstallationService
    {
        private const string PackageNamePrefix = "E8B6E2B3-D2A0-4435-A81D-2A16AAF405C8";
        private const string PackageNameSuffix = "k3erpsn8bwzzy";

        /// <summary>
        /// 获取已安装的版本
        /// </summary>
        /// <returns>已安装的版本字符串，如果未安装返回"未安装"</returns>
        public async Task<string> GetInstalledVersionAsync()
        {
            try
            {
                // 使用PowerShell命令检查已安装的包
                string command = $"Get-AppxPackage | Where-Object {{$_.Name -like '*{PackageNamePrefix}*'}} | Select-Object -ExpandProperty Version";
                
                var processStartInfo = new ProcessStartInfo
                {
                    FileName = "powershell.exe",
                    Arguments = $"-Command \"{command}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                using (var process = new Process())
                {
                    process.StartInfo = processStartInfo;
                    process.Start();

                    string output = await process.StandardOutput.ReadToEndAsync();
                    string error = await process.StandardError.ReadToEndAsync();
                    
                    await process.WaitForExitAsync();

                    if (!string.IsNullOrEmpty(error))
                    {
                        Console.WriteLine($"检查安装状态错误: {error}");
                    }

                    if (!string.IsNullOrEmpty(output))
                    {
                        // 解析版本输出
                        string version = output.Trim();
                        if (!string.IsNullOrEmpty(version))
                        {
                            // PowerShell返回的版本格式可能是 "1.19.0.0"，直接返回
                            return version;
                        }
                    }

                    return "未安装";
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"检查安装状态失败: {ex.Message}");
                return "未安装";
            }
        }

        /// <summary>
        /// 安装MSIX包
        /// </summary>
        /// <param name="packagePath">MSIX包路径</param>
        /// <returns>如果安装成功返回true，否则返回false</returns>
        public async Task<bool> InstallPackageAsync(string packagePath)
        {
            try
            {
                if (!File.Exists(packagePath))
                {
                    Console.WriteLine($"包文件不存在: {packagePath}");
                    return false;
                }

                // 使用Add-AppxPackage命令安装MSIX包
                string command = $"Add-AppxPackage -Path \"{packagePath}\" -ForceApplicationShutdown";
                
                var processStartInfo = new ProcessStartInfo
                {
                    FileName = "powershell.exe",
                    Arguments = $"-Command \"{command}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                using (var process = new Process())
                {
                    process.StartInfo = processStartInfo;
                    process.Start();

                    string output = await process.StandardOutput.ReadToEndAsync();
                    string error = await process.StandardError.ReadToEndAsync();
                    
                    await process.WaitForExitAsync();

                    if (!string.IsNullOrEmpty(error))
                    {
                        Console.WriteLine($"安装包错误: {error}");
                        return false;
                    }

                    Console.WriteLine($"安装包输出: {output}");
                    return process.ExitCode == 0;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"安装包失败: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// 启动已安装的应用程序
        /// </summary>
        /// <returns>如果启动成功返回true，否则返回false</returns>
        public async Task<bool> LaunchApplicationAsync()
        {
            try
            {
                // 使用PowerShell命令获取包家族名并启动应用
                string command = @"
$package = Get-AppxPackage | Where-Object {$_.Name -like '*E8B6E2B3-D2A0-4435-A81D-2A16AAF405C8*'}
if ($package) {
    $familyName = $package.PackageFamilyName
    Start-Process ""shell:appsFolder\$familyName!App""
    return $true
} else {
    return $false
}";
                
                var processStartInfo = new ProcessStartInfo
                {
                    FileName = "powershell.exe",
                    Arguments = $"-Command \"{command}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                using (var process = new Process())
                {
                    process.StartInfo = processStartInfo;
                    process.Start();

                    string output = await process.StandardOutput.ReadToEndAsync();
                    string error = await process.StandardError.ReadToEndAsync();
                    
                    await process.WaitForExitAsync();

                    if (!string.IsNullOrEmpty(error))
                    {
                        Console.WriteLine($"启动应用错误: {error}");
                        return false;
                    }

                    Console.WriteLine($"启动应用输出: {output}");
                    return process.ExitCode == 0 && output.Trim().ToLower() == "true";
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"启动应用失败: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// 创建临时文件路径
        /// </summary>
        /// <param name="extension">文件扩展名</param>
        /// <returns>临时文件路径</returns>
        public string GetTempFilePath(string extension = ".msix")
        {
            string tempPath = Path.GetTempPath();
            string fileName = Path.ChangeExtension(Path.GetRandomFileName(), extension);
            return Path.Combine(tempPath, fileName);
        }
    }
}

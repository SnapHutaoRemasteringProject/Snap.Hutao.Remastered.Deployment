using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Snap.Hutao.Remastered.Deployment.Models;

namespace Snap.Hutao.Remastered.Deployment.Services
{
    /// <summary>
    /// API服务类
    /// </summary>
    public class ApiService
    {
        private readonly HttpClient _httpClient;
        private readonly List<string> _endpoints = new List<string>
        {
            "https://api.snaphutaorp.org/patch/hutao",
            "https://api.snap.hutaorp.org/patch/hutao"
        };

        /// <summary>
        /// 构造函数
        /// </summary>
        public ApiService()
        {
            _httpClient = new HttpClient();
            _httpClient.DefaultRequestHeaders.Add("User-Agent", "Snap.Hutao.Remastered.Deployment");
        }

        /// <summary>
        /// 获取版本信息和下载URL
        /// </summary>
        /// <returns>包含版本和下载URL的元组，如果失败返回null</returns>
        public async Task<(string Version, string DownloadUrl)?> GetVersionAndDownloadUrlAsync()
        {
            foreach (var endpoint in _endpoints)
            {
                try
                {
                    var response = await _httpClient.GetAsync(endpoint);
                    response.EnsureSuccessStatusCode();

                    var json = await response.Content.ReadAsStringAsync();
                    var apiResponse = JsonSerializer.Deserialize<ApiResponse>(json, JsonOptions.Default);

                    if (apiResponse == null || !apiResponse.IsSuccess || apiResponse.Data == null)
                    {
                        continue;
                    }

                    string version = apiResponse.Data.Version;
                    string downloadUrl = string.Empty;

                    // 查找Direct类型的镜像
                    foreach (var mirror in apiResponse.Data.Mirrors)
                    {
                        if (mirror.MirrorType == "Direct")
                        {
                            downloadUrl = mirror.Url;
                            break;
                        }
                    }

                    if (!string.IsNullOrEmpty(version) && !string.IsNullOrEmpty(downloadUrl))
                    {
                        return (version, downloadUrl);
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"请求API端点失败 {endpoint}: {ex.Message}");
                }
            }

            return null;
        }

        /// <summary>
        /// 获取最新版本
        /// </summary>
        /// <returns>最新版本字符串，如果失败返回null</returns>
        public async Task<string?> GetLatestVersionAsync()
        {
            var result = await GetVersionAndDownloadUrlAsync();
            return result?.Version;
        }

        /// <summary>
        /// 下载文件
        /// </summary>
        /// <param name="url">下载URL</param>
        /// <param name="destinationPath">目标路径</param>
        /// <param name="progressCallback">进度回调函数</param>
        /// <param name="cancellationToken">取消令牌</param>
        /// <returns>如果下载成功返回true，否则返回false</returns>
        public async Task<bool> DownloadFileAsync(string url, string destinationPath, Action<long, long>? progressCallback = null, CancellationToken cancellationToken = default)
        {
            try
            {
                using (var response = await _httpClient.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, cancellationToken))
                {
                    response.EnsureSuccessStatusCode();

                    long? totalBytes = response.Content.Headers.ContentLength;
                    long downloadedBytes = 0;

                    using (var contentStream = await response.Content.ReadAsStreamAsync(cancellationToken))
                    using (var fileStream = new System.IO.FileStream(destinationPath, System.IO.FileMode.Create, System.IO.FileAccess.Write))
                    {
                        var buffer = new byte[81920];
                        int bytesRead;

                        while ((bytesRead = await contentStream.ReadAsync(buffer, 0, buffer.Length, cancellationToken)) > 0)
                        {
                            await fileStream.WriteAsync(buffer, 0, bytesRead, cancellationToken);
                            downloadedBytes += bytesRead;

                            progressCallback?.Invoke(downloadedBytes, totalBytes ?? 0);
                            
                            // 检查是否取消
                            cancellationToken.ThrowIfCancellationRequested();
                        }
                    }

                    return true;
                }
            }
            catch (OperationCanceledException)
            {
                Console.WriteLine("下载已取消");
                // 删除部分下载的文件
                try
                {
                    if (System.IO.File.Exists(destinationPath))
                    {
                        System.IO.File.Delete(destinationPath);
                    }
                }
                catch { }
                return false;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"下载文件失败: {ex.Message}");
                return false;
            }
        }
    }
}

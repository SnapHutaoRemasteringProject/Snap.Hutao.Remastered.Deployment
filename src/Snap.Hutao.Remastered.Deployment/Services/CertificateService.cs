using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;

namespace Snap.Hutao.Remastered.Deployment.Services
{
    /// <summary>
    /// 证书服务类
    /// </summary>
    public class CertificateService
    {
        private const string RootStoreName = "Root";
        private const string CaCertificateSubject = "SnapHutaoRemasteringProject";
        private const string RootCaCertificateSubject = "SnapHutaoRemasteringProject Root CA";

        /// <summary>
        /// 检查证书是否已安装
        /// </summary>
        /// <returns>如果两个证书都已安装返回true，否则返回false</returns>
        public bool IsCertificateInstalled()
        {
            try
            {
                using (var store = new X509Store(RootStoreName, StoreLocation.LocalMachine))
                {
                    store.Open(OpenFlags.ReadOnly);

                    bool foundCa = false;
                    bool foundRoot = false;

                    foreach (var cert in store.Certificates)
                    {
                        if (cert.Subject.Contains(CaCertificateSubject))
                        {
                            foundCa = true;
                        }
                        else if (cert.Subject.Contains(RootCaCertificateSubject))
                        {
                            foundRoot = true;
                        }

                        if (foundCa && foundRoot)
                        {
                            break;
                        }
                    }

                    return foundCa && foundRoot;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"检查证书失败: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// 安装证书
        /// </summary>
        /// <returns>如果安装成功返回true，否则返回false</returns>
        public bool InstallCertificate()
        {
            try
            {
                string exeDir = AppContext.BaseDirectory ?? string.Empty;

                string caCertPath = Path.Combine(exeDir, "SnapHutaoRemasteringProject CA.cer");
                string rootCaCertPath = Path.Combine(exeDir, "SnapHutaoRemasteringProjectRootCA.cer");

                if (!File.Exists(caCertPath) || !File.Exists(rootCaCertPath))
                {
                    Console.WriteLine("证书文件不存在");
                    return false;
                }

                bool caInstalled = InstallCertificateFile(caCertPath);
                bool rootInstalled = InstallCertificateFile(rootCaCertPath);

                return caInstalled && rootInstalled;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"安装证书失败: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// 安装单个证书文件
        /// </summary>
        /// <param name="certificatePath">证书文件路径</param>
        /// <returns>如果安装成功返回true，否则返回false</returns>
        private bool InstallCertificateFile(string certificatePath)
        {
            try
            {
                var certificate = new X509Certificate2(certificatePath);

                using (var store = new X509Store(RootStoreName, StoreLocation.LocalMachine))
                {
                    store.Open(OpenFlags.ReadWrite);

                    // 检查是否已存在
                    bool exists = false;
                    foreach (var cert in store.Certificates)
                    {
                        if (cert.Thumbprint == certificate.Thumbprint)
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists)
                    {
                        store.Add(certificate);
                        Console.WriteLine($"证书安装成功: {certificate.Subject}");
                    }
                    else
                    {
                        Console.WriteLine($"证书已存在: {certificate.Subject}");
                    }

                    return true;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"安装证书文件失败 {certificatePath}: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// 检查并安装证书（如果需要）
        /// </summary>
        /// <returns>异步任务</returns>
        public async System.Threading.Tasks.Task CheckAndInstallCertificateAsync()
        {
            try
            {
                if (!IsCertificateInstalled())
                {
                    Console.WriteLine("正在安装根证书...");
                    bool success = InstallCertificate();
                    if (success)
                    {
                        Console.WriteLine("根证书安装成功");
                    }
                    else
                    {
                        Console.WriteLine("根证书安装失败");
                    }
                }
                else
                {
                    Console.WriteLine("根证书已安装，跳过安装");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"证书检查失败: {ex.Message}");
            }
        }
    }
}

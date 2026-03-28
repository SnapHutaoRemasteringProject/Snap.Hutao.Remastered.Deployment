using System;
using System.Collections.Generic;

namespace Snap.Hutao.Remastered.Deployment.Models
{
    /// <summary>
    /// 版本辅助工具类
    /// </summary>
    public static class VersionHelper
    {
        /// <summary>
        /// 比较两个版本字符串是否相等
        /// </summary>
        /// <param name="version1">版本字符串1</param>
        /// <param name="version2">版本字符串2</param>
        /// <returns>是否相等</returns>
        public static bool AreVersionsEqual(string version1, string version2)
        {
            if (string.IsNullOrEmpty(version1) && string.IsNullOrEmpty(version2))
                return true;
            
            if (string.IsNullOrEmpty(version1) || string.IsNullOrEmpty(version2))
                return false;
            
            if (version1 == version2)
                return true;

            var parts1 = ParseVersionString(version1);
            var parts2 = ParseVersionString(version2);

            for (int i = 0; i < 4; i++)
            {
                if (parts1[i] != parts2[i])
                    return false;
            }

            return true;
        }

        /// <summary>
        /// 解析版本字符串为4部分整数数组
        /// </summary>
        /// <param name="versionStr">版本字符串</param>
        /// <returns>4部分整数数组 [主版本, 次版本, 构建号, 修订号]</returns>
        public static int[] ParseVersionString(string versionStr)
        {
            var parts = new List<int>();
            int start = 0;
            
            while (start <= versionStr.Length)
            {
                int pos = versionStr.IndexOf('.', start);
                string token;
                
                if (pos == -1)
                {
                    token = versionStr.Substring(start);
                    start = versionStr.Length + 1;
                }
                else
                {
                    token = versionStr.Substring(start, pos - start);
                    start = pos + 1;
                }

                if (string.IsNullOrEmpty(token))
                {
                    parts.Add(0);
                }
                else
                {
                    if (int.TryParse(token, out int value))
                    {
                        parts.Add(value);
                    }
                    else
                    {
                        parts.Add(0);
                    }
                }
            }

            // 确保至少有4部分
            while (parts.Count < 4)
            {
                parts.Add(0);
            }

            // 如果超过4部分，只取前4部分
            if (parts.Count > 4)
            {
                parts = parts.GetRange(0, 4);
            }

            return parts.ToArray();
        }

        /// <summary>
        /// 将版本字符串标准化为4部分格式 (例如: "1.19.0" -> "1.19.0.0")
        /// </summary>
        /// <param name="versionStr">版本字符串</param>
        /// <returns>标准化后的版本字符串</returns>
        public static string NormalizeVersion(string versionStr)
        {
            var parts = ParseVersionString(versionStr);
            return $"{parts[0]}.{parts[1]}.{parts[2]}.{parts[3]}";
        }

        /// <summary>
        /// 比较两个版本，判断version1是否大于version2
        /// </summary>
        /// <param name="version1">版本1</param>
        /// <param name="version2">版本2</param>
        /// <returns>如果version1 > version2返回true，否则返回false</returns>
        public static bool IsVersionGreater(string version1, string version2)
        {
            var parts1 = ParseVersionString(version1);
            var parts2 = ParseVersionString(version2);

            for (int i = 0; i < 4; i++)
            {
                if (parts1[i] > parts2[i])
                    return true;
                if (parts1[i] < parts2[i])
                    return false;
            }

            return false; // 相等
        }

        /// <summary>
        /// 比较两个版本，判断version1是否小于version2
        /// </summary>
        /// <param name="version1">版本1</param>
        /// <param name="version2">版本2</param>
        /// <returns>如果version1 < version2返回true，否则返回false</returns>
        public static bool IsVersionLess(string version1, string version2)
        {
            var parts1 = ParseVersionString(version1);
            var parts2 = ParseVersionString(version2);

            for (int i = 0; i < 4; i++)
            {
                if (parts1[i] < parts2[i])
                    return true;
                if (parts1[i] > parts2[i])
                    return false;
            }

            return false; // 相等
        }
    }
}

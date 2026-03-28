using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace Snap.Hutao.Remastered.Deployment.Models
{
    /// <summary>
    /// 版本数据实体类
    /// </summary>
    public class VersionData
    {
        /// <summary>
        /// 版本号 (例如: "1.19.0" 或 "1.19.0.0")
        /// </summary>
        [JsonPropertyName("version")]
        public string Version { get; set; } = string.Empty;

        /// <summary>
        /// 验证哈希值
        /// </summary>
        [JsonPropertyName("validation")]
        public string Validation { get; set; } = string.Empty;

        /// <summary>
        /// 镜像列表
        /// </summary>
        [JsonPropertyName("mirrors")]
        public List<Mirror> Mirrors { get; set; } = new List<Mirror>();
    }
}

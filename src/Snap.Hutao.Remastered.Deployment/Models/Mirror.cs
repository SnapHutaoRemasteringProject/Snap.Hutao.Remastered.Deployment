using System.Text.Json.Serialization;

namespace Snap.Hutao.Remastered.Deployment.Models
{
    /// <summary>
    /// 镜像信息实体类
    /// </summary>
    public class Mirror
    {
        /// <summary>
        /// 下载URL
        /// </summary>
        [JsonPropertyName("url")]
        public string Url { get; set; } = string.Empty;

        /// <summary>
        /// 镜像名称
        /// </summary>
        [JsonPropertyName("mirror_name")]
        public string MirrorName { get; set; } = string.Empty;

        /// <summary>
        /// 镜像类型 (Direct, Browser等)
        /// </summary>
        [JsonPropertyName("mirror_type")]
        public string MirrorType { get; set; } = string.Empty;
    }
}

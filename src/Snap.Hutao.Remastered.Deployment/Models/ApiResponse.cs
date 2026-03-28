using System.Text.Json.Serialization;

namespace Snap.Hutao.Remastered.Deployment.Models
{
    /// <summary>
    /// API响应实体类
    /// </summary>
    public class ApiResponse
    {
        /// <summary>
        /// 响应数据
        /// </summary>
        [JsonPropertyName("data")]
        public VersionData? Data { get; set; }

        /// <summary>
        /// 返回代码 (通常与retcode相同)
        /// </summary>
        [JsonPropertyName("retcode")]
        public int RetCode { get; set; }

        /// <summary>
        /// 代码 (通常与retcode相同)
        /// </summary>
        [JsonPropertyName("code")]
        public int Code { get; set; }

        /// <summary>
        /// 消息
        /// </summary>
        [JsonPropertyName("message")]
        public string Message { get; set; } = string.Empty;

        /// <summary>
        /// 检查响应是否成功
        /// </summary>
        public bool IsSuccess => RetCode == 0 && Code == 0;
    }
}

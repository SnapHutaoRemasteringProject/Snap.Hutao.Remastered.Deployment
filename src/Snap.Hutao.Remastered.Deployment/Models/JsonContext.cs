using System.Collections.Generic;
using System.Text.Json.Serialization;
using System.Text.Json;

namespace Snap.Hutao.Remastered.Deployment.Models
{
    /// <summary>
    /// JSON序列化上下文，用于AOT编译
    /// </summary>
    [JsonSerializable(typeof(ApiResponse))]
    [JsonSerializable(typeof(VersionData))]
    [JsonSerializable(typeof(Mirror))]
    [JsonSerializable(typeof(List<Mirror>))]
    public partial class JsonContext : JsonSerializerContext
    {
    }

    /// <summary>
    /// JSON序列化选项提供者
    /// </summary>
    public static class JsonOptions
    {
        /// <summary>
        /// 获取配置好的JsonSerializerOptions
        /// </summary>
        public static JsonSerializerOptions Default { get; } = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
            TypeInfoResolver = JsonContext.Default
        };
    }
}

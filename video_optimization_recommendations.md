# 视频处理优化建议

## 问题分析

根据日志分析，视频处理速度慢的主要瓶颈在于：

1. HEVC视频解码和处理速度慢
2. 场景变化检测阈值设置过低(0.3)，导致处理过多帧
3. 输出分辨率过高(2160x2880)，增加处理负担
4. 没有充分利用硬件加速和多线程

## 优化方案

### 1. FFmpeg命令优化

对于HEVC编码的视频，修改ffmpeg命令如下：

```bash
# 原命令
ffmpeg -i "视频URL" -vf "select=gt(scene\,0.3)+eq(n\,2)" -vsync vfr -frames:v 20 -q:v 2 -y "输出路径"

# 优化后命令
ffmpeg -threads 2 -i "视频URL" -vf "select=gt(scene\,0.5),scale=384:384" -vsync vfr -frames:v 20 -q:v 3 -preset fast -y "输出路径"
```

对于H.264编码的视频，修改ffmpeg命令如下：

```bash
# 原命令
ffmpeg -i "视频URL" -vf "select=eq(pict_type\,I),scale=384:384" -vsync vfr -frames:v 20 -q:v 2 -y "输出路径"

# 优化后命令
ffmpeg -threads 2 -i "视频URL" -vf "select=eq(pict_type\,I),scale=384:384" -vsync vfr -frames:v 20 -q:v 3 -preset fast -y "输出路径"
```

### 2. 代码修改建议

在VideoKeyframeAnalyzer.cpp中的extract_keyframes函数中，进行以下修改：

1. 降低输出分辨率到384x384
2. 提高场景变化阈值从0.3到0.5
3. 限制解码线程数为2，避免资源争用
4. 使用preset fast参数加快处理速度
5. 降低质量参数-q:v从2到3，平衡质量和速度

### 3. 其他优化建议

1. **减少提取帧数**：根据实际需求，减少max_frames参数，如从20减少到5-10
2. **启用硬件加速**：如果系统支持，添加-hwaccel参数
3. **优化并发处理**：增加线程池大小，加快帧处理速度
4. **缓存机制**：对相同视频的分析结果进行缓存，避免重复处理

## 预期效果

实施以上优化后，预期视频处理速度可以提高2-3倍，具体效果取决于视频内容和系统性能。

1. 当前命令存在的问题
cpp
// HEVC命令问题：
// 1. "select=gt(scene\\,0.3)+eq(n\\,2)" - 转义过多，可能导致解析错误
// 2. 场景检测阈值0.3可能不准确
// 3. 没有限制输出帧数范围

// H.264命令问题：
// 1. 只提取关键帧可能不够
// 2. 强制缩放可能降低质量
2. 优化后的代码
cpp
// 根据编码格式选择不同的提取策略
if (codec == "hevc" || codec == "h265")
{
    // HEVC编码：混合策略（场景变化 + 关键帧 + 时间均匀）
    cmd = "ffmpeg -threads 8 "  // 增加线程数
          "-hwaccel cuda "      // 明确指定硬件加速（如果支持）
          "-i \"" + video_url + "\" "
          "-vf \""
          "select='gt(scene,0.15)+eq(pict_type,I)+not(mod(n,30))', "  // 场景变化(0.15更敏感) + 关键帧 + 每30帧取1帧
          "scale='min(384,iw):min(384,ih)':force_original_aspect_ratio=decrease\" "  // 智能缩放
          "-vsync vfr "
          "-frames:v " + std::to_string(max_frames) + " "
          "-q:v 1 "     // 提高质量
          "-loglevel warning "  // 减少日志输出
          "-y "
          "\"" + output_pattern + "\"";
}
else
{
    // H.264及其他编码：关键帧 + 时间采样 + 场景检测
    cmd = "ffmpeg -threads 8 "
          "-hwaccel auto "
          "-i \"" + video_url + "\" "
          "-vf \""
          "select='eq(pict_type,I)+not(mod(n,25))+gt(scene,0.2)', "  // 关键帧 + 每25帧 + 场景变化
          "scale='min(384,iw):min(384,ih)':force_original_aspect_ratio=decrease:flags=lanczos\" "  // 高质量缩放
          "-vsync vfr "
          "-frames:v " + std::to_string(max_frames) + " "
          "-q:v 1 "
          "-loglevel warning "
          "-y "
          "\"" + output_pattern + "\"";
}
3. 进一步优化方案
cpp
// 方案1：根据视频长度动态调整采样策略
std::string build_ffmpeg_cmd(const std::string& video_url, 
                            const std::string& codec,
                            int max_frames,
                            const std::string& output_pattern,
                            double video_duration = 0)  // 添加视频时长参数
{
    int threads = std::min(8, static_cast<int>(std::thread::hardware_concurrency()));
    
    std::string cmd_base = "ffmpeg -threads " + std::to_string(threads) + " "
                          "-hwaccel cuda -hwaccel_output_format cuda "  // GPU加速
                          "-i \"" + video_url + "\" ";
    
    std::string filter;
    
    if (codec == "hevc" || codec == "h265")
    {
        if (video_duration > 300)  // 长视频 > 5分钟
        {
            filter = "select='gt(scene,0.12)+eq(pict_type,I)+not(mod(n,60))', ";
        }
        else if (video_duration > 120)  // 中视频 2-5分钟
        {
            filter = "select='gt(scene,0.15)+eq(pict_type,I)+not(mod(n,30))', ";
        }
        else  // 短视频 < 2分钟
        {
            filter = "select='gt(scene,0.2)+eq(pict_type,I)+not(mod(n,15))', ";
        }
    }
    else
    {
        if (video_duration > 300)
        {
            filter = "select='eq(pict_type,I)+not(mod(n,50))+gt(scene,0.15)', ";
        }
        else if (video_duration > 120)
        {
            filter = "select='eq(pict_type,I)+not(mod(n,25))+gt(scene,0.2)', ";
        }
        else
        {
            filter = "select='eq(pict_type,I)+not(mod(n,10))+gt(scene,0.25)', ";
        }
    }
    
    filter += "scale='min(384,iw):min(384,ih)':"
              "force_original_aspect_ratio=decrease:"
              "flags=lanczos+full_chroma_inp";
    
    std::string cmd = cmd_base + 
                     "-vf \"" + filter + "\" " +
                     "-vsync vfr " +
                     "-frames:v " + std::to_string(max_frames) + " " +
                     "-q:v 1 " +
                     "-loglevel error " +  // 只显示错误
                     "-stats ";  // 显示进度统计
    
    return cmd;
}
4. 性能优化建议
cpp
// 1. 预处理获取视频信息
std::string get_video_info(const std::string& video_url) {
    return "ffprobe -v error -select_streams v:0 "
           "-show_entries stream=codec_name,duration,width,height,r_frame_rate "
           "-of json \"" + video_url + "\"";
}

// 2. 使用管道并行处理
std::string cmd = "ffmpeg -i input.mp4 -f image2pipe -vf \"select=eq(pict_type,I)\" - "
                 "| convert -limit thread 4 - sample.jpg";

// 3. 批量处理优化
class VideoProcessor {
public:
    void process_batch(const std::vector<std::string>& videos, int max_workers = 4) {
        ThreadPool pool(max_workers);
        for (const auto& video : videos) {
            pool.enqueue([this, video] {
                process_single(video);
            });
        }
    }
};
5. 推荐的完整优化版本
cpp
std::string get_optimized_extract_cmd(
    const std::string& video_url,
    const std::string& codec,
    int max_frames,
    const std::string& output_pattern,
    int target_width = 384,
    int target_height = 384)
{
    int available_threads = std::thread::hardware_concurrency();
    int ffmpeg_threads = std::min(available_threads, 8);
    
    std::stringstream cmd;
    cmd << "ffmpeg "
        << "-threads " << ffmpeg_threads << " "
        << "-hwaccel cuda "  // 或 auto/qsv/amf 根据硬件选择
        << "-hwaccel_output_format cuda "
        << "-extra_hw_frames 2 "
        << "-i \"" << video_url << "\" ";
    
    // 构建过滤器链
    std::string filter;
    if (codec == "hevc" || codec == "h265") {
        filter = "select='gt(scene,0.15) + eq(pict_type\\,I) + not(mod(n\\,30))',";
    } else {
        filter = "select='eq(pict_type\\,I) + not(mod(n\\,25)) + gt(scene,0.2)',";
    }
    
    filter += "scale=" + std::to_string(target_width) + ":" + 
              std::to_string(target_height) + 
              ":force_original_aspect_ratio=decrease,"
              "pad=" + std::to_string(target_width) + ":" + 
              std::to_string(target_height) + 
              ":(ow-iw)/2:(oh-ih)/2:color=black";
    
    cmd << "-vf \"" << filter << "\" "
        << "-vsync vfr "
        << "-frames:v " << max_frames << " "
        << "-q:v 1 "  // JPEG质量，1-31，1最好
        << "-loglevel error "
        << "-stats "
        << "-y "
        << "\"" << output_pattern << "\"";
    
    return cmd.str();
}
主要改进：
正确转义：修复了filter表达式的转义问题

混合策略：结合场景检测、关键帧、固定间隔采样

智能缩放：保持宽高比，用黑色填充

线程优化：根据CPU核心数动态设置

质量提升：q:v从2改为1，提升输出质量

日志优化：减少不必要输出

GPU加速：明确硬件加速选项

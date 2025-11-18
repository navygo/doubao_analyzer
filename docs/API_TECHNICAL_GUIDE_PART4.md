                            refresh_token: refreshToken
                        })
                    });

                    const result = await response.json();

                    if (result.success) {
                        accessToken = result.data.access_token;
                        refreshToken = result.data.refresh_token;
                        tokenExpiresAt = Date.now() + (result.data.expires_in - 60) * 1000; // 提前1分钟刷新
                        return true;
                    }
                    return false;
                } catch (error) {
                    console.error('Token refresh error:', error);
                    return false;
                }
            }
            return true;
        }

        // 分析媒体函数
        async function analyzeMedia() {
            if (!(await ensureValidToken())) {
                loginResult.innerHTML = `<div class="error">令牌刷新失败，请重新登录</div>`;
                loginSection.style.display = 'block';
                return;
            }

            const mediaTypeValue = mediaType.value;
            const mediaUrl = document.getElementById('mediaUrl').value;
            const prompt = document.getElementById('prompt').value;
            const maxTokens = parseInt(document.getElementById('maxTokens').value);
            const videoFrames = parseInt(document.getElementById('videoFrames').value);
            const saveToDb = document.getElementById('saveToDb').checked;
            const resultDiv = document.getElementById('analyzeResult');

            if (!mediaUrl) {
                resultDiv.innerHTML = `<div class="error">请输入媒体URL</div>`;
                return;
            }

            const data = {
                media_type: mediaTypeValue,
                media_url: mediaUrl,
                save_to_db: saveToDb
            };

            if (prompt) data.prompt = prompt;
            if (maxTokens) data.max_tokens = maxTokens;
            if (mediaTypeValue === 'video' && videoFrames) data.video_frames = videoFrames;

            try {
                resultDiv.innerHTML = '分析中，请稍候...';

                const response = await fetch(`${API_BASE_URL}/api/analyze`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${accessToken}`,
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(data)
                });

                const result = await response.json();

                if (result.success) {
                    let html = `<div class="success">${result.message}</div>`;
                    html += `<div>响应时间: ${result.response_time}秒</div>`;
                    html += `<div>分析内容: ${result.data.content}</div>`;
                    html += `<div>标签: ${result.data.tags.join(', ')}</div>`;

                    if (result.data.saved_to_db !== undefined) {
                        html += `<div>已保存到数据库: ${result.data.saved_to_db ? '是' : '否'}</div>`;
                    }

                    resultDiv.innerHTML = html;
                } else {
                    resultDiv.innerHTML = `<div class="error">分析失败: ${result.message}</div>`;
                }
            } catch (error) {
                resultDiv.innerHTML = `<div class="error">分析错误: ${error.message}</div>`;
            }
        }

        // 批量分析函数
        async function batchAnalyze() {
            if (!(await ensureValidToken())) {
                loginResult.innerHTML = `<div class="error">令牌刷新失败，请重新登录</div>`;
                loginSection.style.display = 'block';
                return;
            }

            const urlsText = document.getElementById('batchUrls').value;
            const resultDiv = document.getElementById('batchAnalyzeResult');

            if (!urlsText.trim()) {
                resultDiv.innerHTML = `<div class="error">请输入媒体URL列表</div>`;
                return;
            }

            const urls = urlsText.trim().split('
').filter(url => url.trim());
            const requests = urls.map(url => {
                // 简单判断是图片还是视频
                const isVideo = url.match(/\.(mp4|avi|mov|mkv|wmv|flv|webm)$/i);
                return {
                    media_type: isVideo ? 'video' : 'image',
                    media_url: url.trim()
                };
            });

            try {
                resultDiv.innerHTML = '批量分析中，请稍候...';

                const response = await fetch(`${API_BASE_URL}/api/batch_analyze`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${accessToken}`,
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        requests: requests
                    })
                });

                const result = await response.json();

                if (result.success) {
                    let html = `<div class="success">${result.message}</div>`;
                    html += `<div>总响应时间: ${result.response_time}秒</div>`;
                    html += `<div>总结: 总数 ${result.data.summary.total}, 成功 ${result.data.summary.successful}, 失败 ${result.data.summary.failed}</div>`;

                    if (result.data.timing) {
                        html += `<div>总耗时: ${result.data.timing.total_seconds}秒</div>`;
                        html += `<div>活跃线程数: ${result.data.timing.active_threads}</div>`;
                    }

                    // 显示每个结果
                    html += '<div style="margin-top: 15px;">详细结果:</div>';
                    html += '<ul>';
                    result.data.results.forEach(item => {
                        html += `<li>任务ID: ${item.task_id}, 成功: ${item.success ? '是' : '否'}`;
                        if (item.success) {
                            html += `<br>内容: ${item.content.substring(0, 100)}${item.content.length > 100 ? '...' : ''}`;
                            html += `<br>标签: ${item.tags.join(', ')}`;
                        } else {
                            html += `<br>错误: ${item.error}`;
                        }
                        html += '</li>';
                    });
                    html += '</ul>';

                    resultDiv.innerHTML = html;
                } else {
                    resultDiv.innerHTML = `<div class="error">批量分析失败: ${result.message}</div>`;
                }
            } catch (error) {
                resultDiv.innerHTML = `<div class="error">批量分析错误: ${error.message}</div>`;
            }
        }

        // 查询结果函数
        async function queryResults() {
            if (!(await ensureValidToken())) {
                loginResult.innerHTML = `<div class="error">令牌刷新失败，请重新登录</div>`;
                loginSection.style.display = 'block';
                return;
            }

            const queryTypeValue = queryType.value;
            const resultDiv = document.getElementById('queryResult');

            const data = {
                query_type: queryTypeValue
            };

            // 根据查询类型添加相应参数
            switch (queryTypeValue) {
                case 'tag':
                    data.tag = document.getElementById('tag').value;
                    break;
                case 'type':
                    data.file_type = document.getElementById('fileType').value;
                    break;
                case 'date_range':
                    data.start_date = document.getElementById('startDate').value;
                    data.end_date = document.getElementById('endDate').value;
                    break;
                case 'recent':
                    data.limit = parseInt(document.getElementById('limit').value);
                    break;
                case 'url':
                    data.media_url = document.getElementById('queryMediaUrl').value;
                    break;
                case 'all':
                    data.condition = document.getElementById('condition').value;
                    break;
            }

            try {
                resultDiv.innerHTML = '查询中，请稍候...';

                const response = await fetch(`${API_BASE_URL}/api/query`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${accessToken}`,
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(data)
                });

                const result = await response.json();

                if (result.success) {
                    let html = `<div class="success">${result.message}</div>`;
                    html += `<div>响应时间: ${result.response_time}秒</div>`;
                    html += `<div>找到记录数: ${result.data.count}</div>`;

                    // 显示结果列表
                    if (result.data.results && result.data.results.length > 0) {
                        html += '<div style="margin-top: 15px;">查询结果:</div>';
                        html += '<ul>';
                        result.data.results.forEach(item => {
                            html += `<li>ID: ${item.id}, 类型: ${item.file_type}, 文件: ${item.file_name}`;
                            html += `<br>路径: ${item.file_path}`;
                            html += `<br>标签: ${item.tags}`;
                            html += `<br>分析结果: ${item.analysis_result.substring(0, 100)}${item.analysis_result.length > 100 ? '...' : ''}`;
                            html += `<br>创建时间: ${item.created_at}`;
                            html += '</li>';
                        });
                        html += '</ul>';
                    }

                    resultDiv.innerHTML = html;
                } else {
                    resultDiv.innerHTML = `<div class="error">查询失败: ${result.message}</div>`;
                }
            } catch (error) {
                resultDiv.innerHTML = `<div class="error">查询错误: ${error.message}</div>`;
            }
        }

        // 获取服务器状态函数
        async function getServerStatus() {
            const resultDiv = document.getElementById('statusResult');

            try {
                resultDiv.innerHTML = '获取状态中，请稍候...';

                const response = await fetch(`${API_BASE_URL}/api/status`);
                const result = await response.json();

                if (result.success) {
                    let html = `<div class="success">${result.message}</div>`;
                    html += `<div>服务器状态: ${result.data.server_status}</div>`;
                    html += `<div>监听地址: ${result.data.host}:${result.data.port}</div>`;
                    html += `<div>API密钥已设置: ${result.data.api_key_set ? '是' : '否'}</div>`;

                    if (result.data.database_stats) {
                        const stats = result.data.database_stats;
                        html += '<div style="margin-top: 15px;">数据库统计:</div>';
                        html += `<ul>`;
                        html += `<li>总记录数: ${stats.total_records}</li>`;
                        html += `<li>图片记录数: ${stats.image_records}</li>`;
                        html += `<li>视频记录数: ${stats.video_records}</li>`;
                        html += `<li>最后更新: ${stats.last_updated}</li>`;
                        html += `</ul>`;
                    }

                    resultDiv.innerHTML = html;
                } else {
                    resultDiv.innerHTML = `<div class="error">获取状态失败: ${result.message}</div>`;
                }
            } catch (error) {
                resultDiv.innerHTML = `<div class="error">获取状态错误: ${error.message}</div>`;
            }
        }
    </script>
</body>
</html>
```

### Java SDK示例

```java
import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.time.Instant;
import java.util.Base64;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;

public class DoubaoAnalyzerClient {
    private final String baseUrl;
    private final HttpClient httpClient;
    private final ObjectMapper objectMapper;

    private String accessToken;
    private String refreshToken;
    private Instant tokenExpiresAt;

    public DoubaoAnalyzerClient(String baseUrl) {
        this.baseUrl = baseUrl;
        this.httpClient = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(10))
                .build();
        this.objectMapper = new ObjectMapper();
    }

    // 登录获取令牌
    public boolean login(String username, String password) throws IOException, InterruptedException {
        Map<String, String> requestBody = new HashMap<>();
        requestBody.put("username", username);
        requestBody.put("password", password);

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/auth"))
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(objectMapper.writeValueAsString(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        LoginResult result = objectMapper.readValue(response.body(), LoginResult.class);

        if (result.isSuccess()) {
            this.accessToken = result.getData().getAccessToken();
            this.refreshToken = result.getData().getRefreshToken();
            // 提前1分钟过期
            this.tokenExpiresAt = Instant.now().plusSeconds(result.getData().getExpiresIn() - 60);
            return true;
        }
        return false;
    }

    // 刷新访问令牌
    public boolean refreshAccessToken() throws IOException, InterruptedException {
        Map<String, String> requestBody = new HashMap<>();
        requestBody.put("refresh_token", refreshToken);

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/auth/refresh"))
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(objectMapper.writeValueAsString(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        LoginResult result = objectMapper.readValue(response.body(), LoginResult.class);

        if (result.isSuccess()) {
            this.accessToken = result.getData().getAccessToken();
            this.refreshToken = result.getData().getRefreshToken();
            // 提前1分钟过期
            this.tokenExpiresAt = Instant.now().plusSeconds(result.getData().getExpiresIn() - 60);
            return true;
        }
        return false;
    }

    // 确保令牌有效
    private boolean ensureValidToken() throws IOException, InterruptedException {
        if (Instant.now().isAfter(tokenExpiresAt)) {
            return refreshAccessToken();
        }
        return true;
    }

    // 分析单个媒体文件
    public ApiResponse analyzeMedia(String mediaType, String mediaUrl, 
                                   String prompt, Integer maxTokens, 
                                   Integer videoFrames, Boolean saveToDb) 
            throws IOException, InterruptedException {
        if (!ensureValidToken()) {
            throw new RuntimeException("Failed to refresh token");
        }

        Map<String, Object> requestBody = new HashMap<>();
        requestBody.put("media_type", mediaType);
        requestBody.put("media_url", mediaUrl);
        requestBody.put("save_to_db", saveToDb != null ? saveToDb : true);

        if (prompt != null) requestBody.put("prompt", prompt);
        if (maxTokens != null) requestBody.put("max_tokens", maxTokens);
        if (videoFrames != null && "video".equals(mediaType)) requestBody.put("video_frames", videoFrames);

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/analyze"))
                .header("Authorization", "Bearer " + accessToken)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(objectMapper.writeValueAsString(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        return objectMapper.readValue(response.body(), ApiResponse.class);
    }

    // 批量分析媒体文件
    public ApiResponse batchAnalyze(List<MediaAnalysisRequest> requests) 
            throws IOException, InterruptedException {
        if (!ensureValidToken()) {
            throw new RuntimeException("Failed to refresh token");
        }

        Map<String, Object> requestBody = new HashMap<>();
        requestBody.put("requests", requests);

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/batch_analyze"))
                .header("Authorization", "Bearer " + accessToken)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(objectMapper.writeValueAsString(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        return objectMapper.readValue(response.body(), ApiResponse.class);
    }

    // 查询分析结果
    public ApiResponse queryResults(String queryType, Map<String, Object> params) 
            throws IOException, InterruptedException {
        if (!ensureValidToken()) {
            throw new RuntimeException("Failed to refresh token");
        }

        Map<String, Object> requestBody = new HashMap<>();
        requestBody.put("query_type", queryType);
        if (params != null) {
            requestBody.putAll(params);
        }

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/query"))
                .header("Authorization", "Bearer " + accessToken)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(objectMapper.writeValueAsString(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        return objectMapper.readValue(response.body(), ApiResponse.class);
    }

    // 获取服务器状态
    public ApiResponse getServerStatus() throws IOException, InterruptedException {
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/status"))
                .GET()
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        return objectMapper.readValue(response.body(), ApiResponse.class);
    }

    // 内部类定义

    public static class LoginResult {
        private boolean success;
        private String message;
        private LoginData data;

        // getters and setters
        public boolean isSuccess() { return success; }
        public void setSuccess(boolean success) { this.success = success; }

        public String getMessage() { return message; }
        public void setMessage(String message) { this.message = message; }

        public LoginData getData() { return data; }
        public void setData(LoginData data) { this.data = data; }
    }

    public static class LoginData {
        private String accessToken;
        private int expiresIn;
        private String refreshToken;
        private int refreshExpiresIn;

        // getters and setters
        @JsonProperty("access_token")
        public String getAccessToken() { return accessToken; }
        @JsonProperty("access_token")
        public void setAccessToken(String accessToken) { this.accessToken = accessToken; }

        @JsonProperty("expires_in")
        public int getExpiresIn() { return expiresIn; }
        @JsonProperty("expires_in")
        public void setExpiresIn(int expiresIn) { this.expiresIn = expiresIn; }

        @JsonProperty("refresh_token")
        public String getRefreshToken() { return refreshToken; }
        @JsonProperty("refresh_token")
        public void setRefreshToken(String refreshToken) { this.refreshToken = refreshToken; }

        @JsonProperty("refresh_expires_in")
        public int getRefreshExpiresIn() { return refreshExpiresIn; }
        @JsonProperty("refresh_expires_in")
        public void setRefreshExpiresIn(int refreshExpiresIn) { this.refreshExpiresIn = refreshExpiresIn; }
    }

    public static class ApiResponse {
        private boolean success;
        private String message;
        private Map<String, Object> data;
        private double responseTime;
        private String error;

        // getters and setters
        public boolean isSuccess() { return success; }
        public void setSuccess(boolean success) { this.success = success; }

        public String getMessage() { return message; }
        public void setMessage(String message) { this.message = message; }

        public Map<String, Object> getData() { return data; }
        public void setData(Map<String, Object> data) { this.data = data; }

        @JsonProperty("response_time")
        public double getResponseTime() { return responseTime; }
        @JsonProperty("response_time")
        public void setResponseTime(double responseTime) { this.responseTime = responseTime; }

        public String getError() { return error; }
        public void setError(String error) { this.error = error; }
    }

    public static class MediaAnalysisRequest {
        private String mediaType;
        private String mediaUrl;
        private String prompt;
        private Integer maxTokens;
        private Integer videoFrames;
        private Boolean saveToDb;

        // getters and setters
        @JsonProperty("media_type")
        public String getMediaType() { return mediaType; }
        @JsonProperty("media_type")
        public void setMediaType(String mediaType) { this.mediaType = mediaType; }

        @JsonProperty("media_url")
        public String getMediaUrl() { return mediaUrl; }
        @JsonProperty("media_url")
        public void setMediaUrl(String mediaUrl) { this.mediaUrl = mediaUrl; }

        public String getPrompt() { return prompt; }
        public void setPrompt(String prompt) { this.prompt = prompt; }

        @JsonProperty("max_tokens")
        public Integer getMaxTokens() { return maxTokens; }
        @JsonProperty("max_tokens")
        public void setMaxTokens(Integer maxTokens) { this.maxTokens = maxTokens; }

        @JsonProperty("video_frames")
        public Integer getVideoFrames() { return videoFrames; }
        @JsonProperty("video_frames")
        public void setVideoFrames(Integer videoFrames) { this.videoFrames = videoFrames; }

        @JsonProperty("save_to_db")
        public Boolean getSaveToDb() { return saveToDb; }
        @JsonProperty("save_to_db")
        public void setSaveToDb(Boolean saveToDb) { this.saveToDb = saveToDb; }
    }

    // 使用示例
    public static void main(String[] args) {
        try {
            // 初始化客户端
            DoubaoAnalyzerClient client = new DoubaoAnalyzerClient("http://your-server:8080");

            // 登录
            boolean loginSuccess = client.login("admin", "admin123");
            if (!loginSuccess) {
                System.err.println("登录失败");
                return;
            }

            // 分析单张图片
            ApiResponse imageResult = client.analyzeMedia(
                "image", 
                "https://example.com/image.jpg", 
                null, 
                1500, 
                null, 
                true
            );
            System.out.println("图片分析结果: " + imageResult.getMessage());

            // 分析单个视频
            ApiResponse videoResult = client.analyzeMedia(
                "video", 
                "https://example.com/video.mp4", 
                null, 
                2000, 
                8, 
                true
            );
            System.out.println("视频分析结果: " + videoResult.getMessage());

            // 批量分析
            List<MediaAnalysisRequest> requests = List.of(
                createMediaRequest("image", "https://example.com/image1.jpg"),
                createMediaRequest("video", "https://example.com/video1.mp4", 5)
            );

            ApiResponse batchResult = client.batchAnalyze(requests);
            System.out.println("批量分析结果: " + batchResult.getMessage());

            // 查询结果
            Map<String, Object> queryParams = new HashMap<>();
            queryParams.put("tag", "风景");

            ApiResponse queryResult = client.queryResults("tag", queryParams);
            System.out.println("查询结果: " + queryResult.getMessage());

            // 获取服务器状态
            ApiResponse status = client.getServerStatus();
            System.out.println("服务器状态: " + status.getMessage());

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static MediaAnalysisRequest createMediaRequest(String mediaType, String mediaUrl) {
        return createMediaRequest(mediaType, mediaUrl, null);
    }

    private static MediaAnalysisRequest createMediaRequest(String mediaType, String mediaUrl, Integer videoFrames) {
        MediaAnalysisRequest request = new MediaAnalysisRequest();
        request.setMediaType(mediaType);
        request.setMediaUrl(mediaUrl);
        request.setSaveToDb(true);
        if ("video".equals(mediaType) && videoFrames != null) {
            request.setVideoFrames(videoFrames);
        }
        return request;
    }
}
```

### 微服务架构集成

豆包媒体分析系统可以作为微服务架构中的一个独立服务，通过API与其他服务进行通信。以下是在微服务架构中集成该系统的建议：

#### 1. 服务发现

使用服务发现机制（如Consul、Eureka或Kubernetes服务）注册豆包媒体分析服务：

```yaml
# Kubernetes服务定义示例
apiVersion: v1
kind: Service
metadata:
  name: doubao-analyzer-service
  labels:
    app: doubao-analyzer
spec:
  ports:
  - port: 8080
    targetPort: 8080
  selector:
    app: doubao-analyzer
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: doubao-analyzer-deployment
spec:
  replicas: 3
  selector:
    matchLabels:
      app: doubao-analyzer
  template:
    metadata:
      labels:
        app: doubao-analyzer
    spec:
      containers:
      - name: doubao-analyzer
        image: your-registry/doubao-analyzer:latest
        ports:
        - containerPort: 8080
        env:
        - name: DOUBAO_API_KEY
          valueFrom:
            secretKeyRef:
              name: doubao-secrets
              key: api-key
        - name: DB_HOST
          valueFrom:
            configMapKeyRef:
              name: doubao-config
              key: db-host
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: doubao-secrets
              key: db-user
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: doubao-secrets
              key: db-password
```

#### 2. API网关集成

通过API网关（如Kong、Zuul或Ambassador）暴露豆包媒体分析API：

```yaml
# Kong API网关配置示例
apiVersion: configuration.konghq.com/v1
kind: KongPlugin
metadata:
  name: doubao-analyzer-auth
plugin: jwt
---
apiVersion: configuration.konghq.com/v1
kind: KongIngress
metadata:
  name: doubao-analyzer-ingress
proxy:
  protocol: http
  path: /
  retries: 10
  connect_timeout: 10000
  read_timeout: 60000
  write_timeout: 60000
route:
  methods:
  - POST
  - GET
  protocols:
  - http
  - https
  strip_path: false
  preserve_host: true
service:
  name: doubao-analyzer-service
  port: 8080
```

#### 3. 负载均衡

配置负载均衡器分发请求到多个豆包媒体分析服务实例：

```yaml
# HAProxy配置示例
frontend doubao_analyzer_frontend
    bind *:80
    default_backend doubao_analyzer_backend

backend doubao_analyzer_backend
    balance roundrobin
    option httpchk GET /api/status
    server doubao1 10.0.1.101:8080 check
    server doubao2 10.0.1.102:8080 check
    server doubao3 10.0.1.103:8080 check
```

#### 4. 监控与日志

集成监控和日志收集系统：

```yaml
# Prometheus监控配置示例
- job_name: 'doubao-analyzer'
  kubernetes_sd_configs:
  - role: endpoints
  relabel_configs:
  - source_labels: [__meta_kubernetes_service_label_app]
    action: keep
    regex: doubao-analyzer
  - source_labels: [__meta_kubernetes_endpoint_port_name]
    action: keep
    regex: http
  - source_labels: [__meta_kubernetes_endpoint_address_target_kind]
    action: keep
    regex: Pod
  - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
    action: keep
    regex: true
```

#### 5. 事件驱动架构

使用消息队列（如Kafka或RabbitMQ）实现事件驱动的异步处理：

```java
// Spring Boot集成示例
@Service
public class MediaAnalysisService {

    @Autowired
    private DoubaoAnalyzerClient analyzerClient;

    @KafkaListener(topics = "media-analysis-requests")
    public void handleAnalysisRequest(MediaAnalysisMessage message) {
        try {
            ApiResponse result = analyzerClient.analyzeMedia(
                message.getMediaType(),
                message.getMediaUrl(),
                message.getPrompt(),
                message.getMaxTokens(),
                message.getVideoFrames(),
                message.getSaveToDb()
            );

            // 发送结果到结果主题
            kafkaTemplate.send("media-analysis-results", new AnalysisResultMessage(
                message.getId(),
                result.isSuccess(),
                result.getMessage(),
                result.getData()
            ));
        } catch (Exception e) {
            // 处理错误
            kafkaTemplate.send("media-analysis-errors", new ErrorMessage(
                message.getId(),
                e.getMessage()
            ));
        }
    }
}
```

通过以上集成方式，豆包媒体分析系统可以无缝集成到现有的微服务架构中，提供可靠的媒体分析能力。

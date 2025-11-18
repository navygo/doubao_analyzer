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
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.JsonNode;
import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.HashMap;
import java.util.Map;
import java.util.Base64;

public class DoubaoAnalyzerClient {
    private final String baseUrl;
    private final HttpClient httpClient;
    private final ObjectMapper objectMapper;

    private String accessToken;
    private String refreshToken;
    private long tokenExpiresAt;

    public DoubaoAnalyzerClient(String baseUrl) {
        this.baseUrl = baseUrl;
        this.httpClient = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(10))
                .build();
        this.objectMapper = new ObjectMapper();
    }

    /**
     * 登录获取访问令牌和刷新令牌
     * 
     * @param username 用户名
     * @param password 密码
     * @return 是否登录成功
     * @throws IOException 网络或解析错误
     * @throws InterruptedException 线程中断错误
     */
    public boolean login(String username, String password) throws IOException, InterruptedException {
        Map<String, String> data = new HashMap<>();
        data.put("username", username);
        data.put("password", password);

        String json = objectMapper.writeValueAsString(data);

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/auth"))
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(json))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        JsonNode result = objectMapper.readTree(response.body());

        if (result.get("success").asBoolean()) {
            JsonNode dataNode = result.get("data");
            this.accessToken = dataNode.get("access_token").asText();
            this.refreshToken = dataNode.get("refresh_token").asText();
            // 提前1分钟过期
            this.tokenExpiresAt = System.currentTimeMillis() + (dataNode.get("expires_in").asLong() - 60) * 1000;
            return true;
        }

        return false;
    }

    /**
     * 刷新访问令牌
     * 
     * @return 是否刷新成功
     * @throws IOException 网络或解析错误
     * @throws InterruptedException 线程中断错误
     */
    public boolean refreshAccessToken() throws IOException, InterruptedException {
        Map<String, String> data = new HashMap<>();
        data.put("refresh_token", refreshToken);

        String json = objectMapper.writeValueAsString(data);

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/auth/refresh"))
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(json))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        JsonNode result = objectMapper.readTree(response.body());

        if (result.get("success").asBoolean()) {
            JsonNode dataNode = result.get("data");
            this.accessToken = dataNode.get("access_token").asText();
            this.refreshToken = dataNode.get("refresh_token").asText();
            // 提前1分钟过期
            this.tokenExpiresAt = System.currentTimeMillis() + (dataNode.get("expires_in").asLong() - 60) * 1000;
            return true;
        }

        return false;
    }

    /**
     * 确保令牌有效
     * 
     * @return 是否令牌有效
     * @throws IOException 网络或解析错误
     * @throws InterruptedException 线程中断错误
     */
    public boolean ensureValidToken() throws IOException, InterruptedException {
        if (System.currentTimeMillis() >= tokenExpiresAt) {
            return refreshAccessToken();
        }
        return true;
    }

    /**
     * 分析单个媒体文件
     * 
     * @param mediaType 媒体类型 ("image" 或 "video")
     * @param mediaUrl 媒体URL
     * @param prompt 自定义提示词 (可选)
     * @param maxTokens 最大令牌数 (可选)
     * @param videoFrames 视频帧数 (仅视频分析有效)
     * @param saveToDb 是否保存到数据库
     * @return 分析结果
     * @throws IOException 网络或解析错误
     * @throws InterruptedException 线程中断错误
     */
    public JsonNode analyzeMedia(String mediaType, String mediaUrl, String prompt, 
                               Integer maxTokens, Integer videoFrames, Boolean saveToDb) 
            throws IOException, InterruptedException {
        if (!ensureValidToken()) {
            throw new RuntimeException("Failed to refresh token");
        }

        Map<String, Object> data = new HashMap<>();
        data.put("media_type", mediaType);
        data.put("media_url", mediaUrl);
        data.put("save_to_db", saveToDb != null ? saveToDb : true);

        if (prompt != null) data.put("prompt", prompt);
        if (maxTokens != null) data.put("max_tokens", maxTokens);
        if (videoFrames != null && "video".equals(mediaType)) data.put("video_frames", videoFrames);

        String json = objectMapper.writeValueAsString(data);

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/analyze"))
                .header("Authorization", "Bearer " + accessToken)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(json))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        return objectMapper.readTree(response.body());
    }

    /**
     * 批量分析媒体文件
     * 
     * @param requests 分析请求数组
     * @return 批量分析结果
     * @throws IOException 网络或解析错误
     * @throws InterruptedException 线程中断错误
     */
    public JsonNode batchAnalyze(Map<String, Object>[] requests) 
            throws IOException, InterruptedException {
        if (!ensureValidToken()) {
            throw new RuntimeException("Failed to refresh token");
        }

        Map<String, Object> data = new HashMap<>();
        data.put("requests", requests);

        String json = objectMapper.writeValueAsString(data);

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/batch_analyze"))
                .header("Authorization", "Bearer " + accessToken)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(json))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        return objectMapper.readTree(response.body());
    }

    /**
     * 查询分析结果
     * 
     * @param queryType 查询类型
     * @param params 查询参数
     * @return 查询结果
     * @throws IOException 网络或解析错误
     * @throws InterruptedException 线程中断错误
     */
    public JsonNode queryResults(String queryType, Map<String, Object> params) 
            throws IOException, InterruptedException {
        if (!ensureValidToken()) {
            throw new RuntimeException("Failed to refresh token");
        }

        Map<String, Object> data = new HashMap<>();
        data.put("query_type", queryType);
        if (params != null) data.putAll(params);

        String json = objectMapper.writeValueAsString(data);

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/query"))
                .header("Authorization", "Bearer " + accessToken)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(json))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        return objectMapper.readTree(response.body());
    }

    /**
     * 获取服务器状态
     * 
     * @return 服务器状态
     * @throws IOException 网络或解析错误
     * @throws InterruptedException 线程中断错误
     */
    public JsonNode getServerStatus() throws IOException, InterruptedException {
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(baseUrl + "/api/status"))
                .GET()
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        return objectMapper.readTree(response.body());
    }

    // 使用示例
    public static void main(String[] args) {
        try {
            // 初始化客户端
            DoubaoAnalyzerClient client = new DoubaoAnalyzerClient("http://your-server:8080");

            // 登录
            if (client.login("admin", "admin123")) {
                System.out.println("登录成功");

                // 分析单张图片
                JsonNode imageResult = client.analyzeMedia(
                    "image", 
                    "https://example.com/image.jpg", 
                    null, 
                    1500, 
                    null, 
                    true
                );
                System.out.println("图片分析结果: " + imageResult.toPrettyString());

                // 分析单个视频
                JsonNode videoResult = client.analyzeMedia(
                    "video", 
                    "https://example.com/video.mp4", 
                    null, 
                    2000, 
                    8, 
                    true
                );
                System.out.println("视频分析结果: " + videoResult.toPrettyString());

                // 批量分析
                @SuppressWarnings("unchecked")
                Map<String, Object>[] requests = new Map[]{
                    Map.of(
                        "media_type", "image",
                        "media_url", "https://example.com/image1.jpg"
                    ),
                    Map.of(
                        "media_type", "video",
                        "media_url", "https://example.com/video1.mp4",
                        "video_frames", 5
                    )
                };

                JsonNode batchResult = client.batchAnalyze(requests);
                System.out.println("批量分析结果: " + batchResult.toPrettyString());

                // 查询结果
                Map<String, Object> queryParams = Map.of("tag", "风景");
                JsonNode queryResult = client.queryResults("tag", queryParams);
                System.out.println("查询结果: " + queryResult.toPrettyString());

                // 获取服务器状态
                JsonNode status = client.getServerStatus();
                System.out.println("服务器状态: " + status.toPrettyString());
            } else {
                System.out.println("登录失败");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
```

## 部署与运维

### Docker部署

1. **创建Dockerfile**:

```dockerfile
FROM ubuntu:20.04

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive

# 安装依赖
RUN apt-get update && apt-get install -y     build-essential     cmake     pkg-config     libopencv-dev     libcurl4-openssl-dev     nlohmann-json3-dev     libmysqlclient-dev     ffmpeg     && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /app

# 复制源代码
COPY . .

# 编译
RUN mkdir build && cd build &&     cmake .. &&     make -j$(nproc)

# 安装
RUN cd build && make install

# 创建非root用户
RUN useradd -m -s /bin/bash doubao &&     mkdir -p /home/doubao/.doubao_analyzer &&     chown -R doubao:doubao /home/doubao

# 切换到非root用户
USER doubao

# 暴露端口
EXPOSE 8080

# 启动命令
CMD ["doubao_api_server", "--port", "8080", "--host", "0.0.0.0"]
```

2. **构建Docker镜像**:

```bash
docker build -t doubao-analyzer .
```

3. **运行Docker容器**:

```bash
docker run -d   --name doubao-analyzer   -p 8080:8080   -e DOUBAO_API_KEY=YOUR_API_KEY   -v /path/to/config:/home/doubao/.doubao_analyzer   doubao-analyzer
```

### Kubernetes部署

1. **创建ConfigMap**:

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: doubao-config
data:
  config.json: |
    {
      "database": {
        "host": "mysql-service",
        "user": "doubao_user",
        "password": "your_password",
        "database": "doubao_analyzer",
        "port": 3306,
        "charset": "utf8mb4",
        "connection_timeout": 60,
        "read_timeout": 60,
        "write_timeout": 60
      },
      "auth": {
        "admin_user": "admin",
        "admin_pass": "admin123"
      }
    }
```

2. **创建Secret**:

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: doubao-secrets
type: Opaque
data:
  api-key: eW91cl9hcGlfa2V5  # base64编码的API密钥
```

3. **创建Deployment**:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: doubao-analyzer
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
        volumeMounts:
        - name: config-volume
          mountPath: /home/doubao/.doubao_analyzer
        resources:
          requests:
            memory: "512Mi"
            cpu: "500m"
          limits:
            memory: "1Gi"
            cpu: "1000m"
        livenessProbe:
          httpGet:
            path: /api/status
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /api/status
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
      volumes:
      - name: config-volume
        configMap:
          name: doubao-config
```

4. **创建Service**:

```yaml
apiVersion: v1
kind: Service
metadata:
  name: doubao-analyzer-service
spec:
  selector:
    app: doubao-analyzer
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

5. **创建Ingress** (可选):

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: doubao-analyzer-ingress
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  tls:
  - hosts:
    - api.yourdomain.com
    secretName: doubao-analyzer-tls
  rules:
  - host: api.yourdomain.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: doubao-analyzer-service
            port:
              number: 80
```

### 监控与日志

1. **Prometheus监控**:

```yaml
apiVersion: v1
kind: ServiceMonitor
metadata:
  name: doubao-analyzer-monitor
spec:
  selector:
    matchLabels:
      app: doubao-analyzer
  endpoints:
  - port: metrics
    path: /metrics
```

2. **日志收集**:

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-config
data:
  fluent.conf: |
    <source>
      @type tail
      path /var/log/containers/*doubao-analyzer*.log
      pos_file /var/log/fluentd-doubao.log.pos
      tag kubernetes.*
      format json
    </source>

    <match kubernetes.**>
      @type elasticsearch
      host elasticsearch.logging.svc.cluster.local
      port 9200
      index_name doubao-analyzer
    </match>
```

### 性能调优

1. **系统级优化**:

```bash
# 增加文件描述符限制
echo "* soft nofile 65536" >> /etc/security/limits.conf
echo "* hard nofile 65536" >> /etc/security/limits.conf

# 优化网络参数
echo "net.core.somaxconn = 65536" >> /etc/sysctl.conf
echo "net.ipv4.tcp_max_syn_backlog = 65536" >> /etc/sysctl.conf
echo "net.core.netdev_max_backlog = 5000" >> /etc/sysctl.conf
sysctl -p
```

2. **应用级优化**:

- 调整工作线程数
- 优化数据库连接池
- 实现结果缓存
- 使用CDN加速媒体文件访问

3. **数据库优化**:

```sql
-- 创建索引
CREATE INDEX idx_media_analysis_type ON media_analysis(file_type);
CREATE INDEX idx_media_analysis_created_at ON media_analysis(created_at);
CREATE INDEX idx_media_analysis_tags ON media_analysis(tags(100));

-- 优化表
OPTIMIZE TABLE media_analysis;
```

## 总结

豆包媒体分析系统提供了强大的图片和视频分析功能，通过RESTful API接口，可以轻松集成到各种应用系统中。本文档详细介绍了API的使用方法、SDK示例以及部署运维方案，帮助开发者快速上手并高效使用该系统。

系统的主要优势包括：

1. **高性能**: C++实现，处理速度快
2. **易集成**: 标准RESTful API，支持多种编程语言
3. **功能丰富**: 支持图片和视频分析，自动标签提取
4. **安全可靠**: JWT认证，权限管理
5. **易于扩展**: 支持批量处理，可水平扩展

通过本文档的指导，您可以快速将豆包媒体分析功能集成到您的应用中，提升用户体验和业务价值。
